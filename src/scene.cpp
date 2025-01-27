//
// Created by fynn on 21.12.22.
//

#include <cmath>

#include <vtkNew.h>
#include <vtkNamedColors.h>

#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderer.h>

#include <vtkPolyDataMapper.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>

#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <vtkCallbackCommand.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCameraOrientationWidget.h>

#include "scene.hpp"
#include "convenience.hpp"


unsigned short MAX_USHORT = -1;
unsigned short USHORT_FAT = 14704;
unsigned short USHORT_WATER = 16384;
unsigned short USHORT_TISSUE = 19584;
unsigned short USHORT_CANCELLOUS_BONE = 21984;
unsigned short USHORT_CORTICAL_BONE_LOWER = 24384;
unsigned short USHORT_CORTICAL_BONE_UPPER = 46784;


void scene::set_transparency_window(unsigned short center, unsigned short width,
                                    double min_opacity, double max_opacity) {

    unsigned short reach = std::div(width, 2).quot;
    unsigned short distance = reach;

    bool needs_correction = false;

    unsigned short lower_edge = center - reach;
    // check underflow
    if (lower_edge > center) {
        distance = center - 1;
        needs_correction = true;
    }

    unsigned short upper_edge = center + reach;
    // check overflow
    if (upper_edge < center) {
        distance = MAX_USHORT - center - 1;
        needs_correction = true;
    }

    if (needs_correction)
        reach = distance;

    transparency_threshold = center - reach;
    opacity_threshold = center + reach;

    std::string projection_mode = get_projection_mode();

    // special handling for maximum intensity and additive projections
    if (projection_mode == "Maximum Intensity" || projection_mode == "Additive") {
        min_opacity /= 2;
        max_opacity /= 2;
    }

    vtkNew<vtkPiecewiseFunction> opacity;
    opacity->AddPoint(transparency_threshold, min_opacity);
    opacity->AddPoint(opacity_threshold, max_opacity);

    // special handling for iso surfaces projection
    if (get_projection_mode() == "Iso Surface") {
        opacity->RemoveAllPoints();
        for (int i = 1; i < iso_steps; ++i) {
            double relative_intensity =
                    ((double) (i * iso_step_width) / MAX_USHORT)
                    * (opacity_threshold - transparency_threshold)
                    + transparency_threshold;
            opacity->AddPoint(relative_intensity, 1. / (iso_steps - i));
        }
    }

    volume->GetProperty()->SetScalarOpacity(opacity);
}

void keypress_callback(vtkObject *caller, long unsigned int event_id, void *client_data, void *call_data) {
    auto *interactor = reinterpret_cast<vtkRenderWindowInteractor *>(caller);
    auto *scene = reinterpret_cast<class scene *>(client_data);

    std::string key = interactor->GetKeySym();

    // Left/Right change transparency_window_center
    unsigned short center = scene->get_transparency_window_center();
    unsigned short adjusted_center = center;
    unsigned short center_step = std::div(MAX_USHORT, 50).quot;
    if (key == "Left") {
        adjusted_center = center - center_step;
        // check underflow and edge
        if (adjusted_center < 16 || center < adjusted_center)
            adjusted_center = 16;
    } else if (key == "Right") {
        adjusted_center = center + center_step;
        // check overflow and edge
        if (adjusted_center < center || MAX_USHORT - 16 < adjusted_center)
            adjusted_center = MAX_USHORT - 16;
    }

    // Up/Down change transparency_window_width
    unsigned short width = scene->get_transparency_window_width();
    unsigned short adjusted_width = width;
    unsigned short width_step = std::div(MAX_USHORT, 100).quot;
    if (key == "Down") {
        adjusted_width = width - width_step;
        // check underflow
        if (width < 32 || width < adjusted_width)
            adjusted_width = 32;
    } else if (key == "Up") {
        adjusted_width = width + width_step;
        // check overflow
        if (adjusted_width < width)
            adjusted_width = MAX_USHORT - 1;
    }

    scene->set_transparency_window(adjusted_center, adjusted_width);

    // Prior/Next change min/max opacity (can't work, need four buttons)
    // if (key == "Prior") {} else if (key == "Next") {}

    // p toggles projection modes
    if (key == "p")
        scene->toggle_projection_mode();

    // r resets camera position
    if (key == "r")
        scene->reset_camera();

    scene->compute_legend();
    scene->refresh();

    // q quits
    if (key == "q") {
        scene->quit();
        exit(0);
    }
}

scene::scene(unsigned short *data_ptr, unsigned short x, unsigned short y, unsigned short z, std::string meta_data) {
    // init image data
    int dt = VTK_UNSIGNED_SHORT;
    int dt_bytes = sizeof(unsigned short);

    size_t slice_bytes = dt_bytes * x * y;

    image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(x, y, z);

    image->AllocateScalars(dt, 1);
    image->SetSpacing(1, 1, 1);

    for (int i = 0; i < z; i++)
        memcpy(image->GetScalarPointer(0, 0, i), data_ptr + (x * y * i), slice_bytes);

    this->meta_data = meta_data;

    colors = vtkSmartPointer<vtkNamedColors>::New();

    vtkNew<vtkVolumeProperty> property;
    property->ShadeOn();
    property->SetInterpolationTypeToLinear();

    mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    mapper->SetInputData(image);
    mapper->SetBlendModeToComposite();
    projection_mode = 0;

    volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);
    volume->SetProperty(property);

    vtkNew<vtkColorTransferFunction> color;
    color->RemoveAllPoints();
    color->AddRGBPoint(USHORT_FAT,
                       colors->GetColor3d("Flesh").GetData()[0],
                       colors->GetColor3d("Flesh").GetData()[1],
                       colors->GetColor3d("Flesh").GetData()[2]);
    color->AddRGBPoint(USHORT_WATER,
                       colors->GetColor3d("Blood").GetData()[0],
                       colors->GetColor3d("Blood").GetData()[1],
                       colors->GetColor3d("Blood").GetData()[2]);
    color->AddRGBPoint(USHORT_TISSUE,
                       colors->GetColor3d("Flesh").GetData()[0],
                       colors->GetColor3d("Flesh").GetData()[1],
                       colors->GetColor3d("Flesh").GetData()[2]);
    color->AddRGBPoint(USHORT_CANCELLOUS_BONE,
                       colors->GetColor3d("Ivory").GetData()[0],
                       colors->GetColor3d("Ivory").GetData()[1],
                       colors->GetColor3d("Ivory").GetData()[2]);
    color->AddRGBPoint(USHORT_CORTICAL_BONE_UPPER,
                       colors->GetColor3d("Ivory").GetData()[0],
                       colors->GetColor3d("Ivory").GetData()[1],
                       colors->GetColor3d("Ivory").GetData()[2]);

    volume->GetProperty()->SetColor(color);

    // set iso surface values in ragular ranges
    iso_steps = 16;
    iso_step_width = std::div(MAX_USHORT, iso_steps - 1).quot;
    for (int i = 1; i < iso_steps; ++i)
        volume->GetProperty()->GetIsoSurfaceValues()->SetValue(i, i * iso_step_width);

    camera = vtkSmartPointer<vtkCamera>::New();

    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(colors->GetColor3d("DarkSlateGray").GetData());
    renderer->AddVolume(volume);
    renderer->SetActiveCamera(camera);
    renderer->ResetCameraClippingRange();
    renderer->ResetCamera();

    // set a decent view
    camera->Elevation(-90);
    camera->SetViewUp(0, 0, 1);
    camera->Azimuth(-30);
    camera->Elevation(20);

    // remember camera position for resetting
    double *cpos = camera->GetPosition();
    cx = cpos[0];
    cy = cpos[1];
    cz = cpos[2];
    cd = camera->GetDistance();

    // get a window, add the renderer, set a slightly bigger initial size
    window = vtkSmartPointer<vtkRenderWindow>::New();
    window->AddRenderer(renderer);
    window->SetSize(800, 800);

    interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(window);

    // get a widget for nice camera manipulation
    camera_widget = vtkSmartPointer<vtkCameraOrientationWidget>::New();
    camera_widget->SetParentRenderer(renderer);
    camera_widget->SetInteractor(interactor);

    vtkSmartPointer<vtkCallbackCommand> cb = vtkSmartPointer<vtkCallbackCommand>::New();
    cb->SetCallback(keypress_callback);
    cb->SetClientData(this);

    interactor->AddObserver(vtkCommand::KeyPressEvent, cb);
    style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);

    info_text = vtkSmartPointer<vtkTextActor>::New();
    info_text->SetInput("");
    info_text->SetPosition(5, 5);
    info_text->GetTextProperty()->SetFontSize(12);
    info_text->GetTextProperty()->SetColor(colors->GetColor3d("Gold").GetData());
    renderer->AddActor2D(info_text);
}

int scene::render() {
    reset_camera();
    set_transparency_window(MAX_USHORT / 2, MAX_USHORT / 2);
    compute_legend();
    camera_widget->On();
    interactor->Initialize();
    interactor->Start();
    return EXIT_SUCCESS;
}

unsigned short scene::get_transparency_window_center() {
    return std::div((transparency_threshold + opacity_threshold), 2).quot;
}

unsigned short scene::get_transparency_window_width() {
    return (opacity_threshold - transparency_threshold) + 1;
}

void scene::refresh() {
    window->Render();
}

void scene::quit() {
    interactor->RemoveAllObservers();
    window->Finalize();
    interactor->TerminateApp();

    colors->Delete();
    camera_widget->Delete();
    renderer->Delete();
    window->Delete();
    interactor->Delete();
    style->Delete();
    camera->Delete();

    image->Delete();
    volume->Delete();
    mapper->Delete();
    info_text->Delete();
}

std::string scene::toggle_projection_mode() {
    projection_mode = (projection_mode + 1) % 4;
    std::string mode_string;

    switch (projection_mode) {
        case 0:
            mapper->SetBlendModeToComposite();
            mode_string.append("Composite");
            break;
        case 1:
            mapper->SetBlendModeToMaximumIntensity();
            mode_string.append("Maximum Intensity");
            break;
        case 2:
            mapper->SetBlendModeToIsoSurface();
            mode_string.append("Iso Surface");
            break;
        case 3:
            mapper->SetBlendModeToAdditive();
            mode_string.append("Additive");
            break;
    }

    return mode_string;
}

std::string scene::get_projection_mode() const {
    std::string mode_string;
    switch (projection_mode) {
        case 0:
            mode_string.append("Composite");
            break;
        case 1:
            mode_string.append("Maximum Intensity");
            break;
        case 2:
            mode_string.append("Iso Surface");
            break;
        case 3:
            mode_string.append("Additive");
            break;
    }
    return mode_string;
}

void scene::compute_legend() {
    std::string info = meta_data + compute_window_info_text();
    info_text->SetInput(info.data());
}

std::string scene::compute_window_info_text() {
    std::string info_string;
    info_string
            .append("Center: ")
            .append(std::to_string(std::div(get_transparency_window_center(), 16).quot  - 1024))
            .append("\n")

            .append("Width: ")
            .append(std::to_string(std::div(get_transparency_window_width(), 16).quot))
            .append("\n")

            .append("Projection Mode: ")
            .append(get_projection_mode());

    return info_string;
}

void scene::reset_camera() {
    camera->SetPosition(cx, cy, cz);
    camera->SetDistance(cd);
    camera->SetViewUp(0, 0, 1);
}
