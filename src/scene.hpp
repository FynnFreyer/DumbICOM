//
// Created by fynn on 21.12.22.
//

#ifndef ABGABE_CG_VIS_SCENE_HPP
#define ABGABE_CG_VIS_SCENE_HPP


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
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include <vtkCallbackCommand.h>
#include <vtkTextActor.h>
#include <vtkCameraOrientationWidget.h>

#include "scene.hpp"
#include "convenience.hpp"


class scene {
public:
    scene(unsigned short *data_ptr,
          unsigned short x, unsigned short y, unsigned short z,
          std::string meta_data = "");

    scene(scene &from) = default;

    ~scene() = default;

    int render();

    unsigned short get_transparency_window_center();

    unsigned short get_transparency_window_width();

    void set_transparency_window(unsigned short center, unsigned short width, double min_opacity = 0, double max_opacity = 1);

    std::string get_projection_mode() const;

    std::string toggle_projection_mode();

    void compute_legend();

    std::string compute_window_info_text();

    void reset_camera();

    void refresh();

    void quit();

protected:
    // the transparency/opacity thresholds determine
    // under which value we achieve max transparency/opacity
    unsigned short transparency_threshold;
    unsigned short opacity_threshold;

    unsigned short iso_steps;
    unsigned short iso_step_width;

    std::string meta_data;
    char projection_mode;

    vtkSmartPointer<vtkNamedColors> colors;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkRenderWindow> window;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkInteractorStyle> style;

    vtkSmartPointer<vtkCamera> camera;
    vtkSmartPointer<vtkCameraOrientationWidget> camera_widget;

    vtkSmartPointer<vtkImageData> image;
    vtkSmartPointer<vtkVolume> volume;
    vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper> mapper;
    vtkSmartPointer<vtkTextActor> info_text;


    double cx;
    double cy;
    double cz;
    double cd;
};


#endif //ABGABE_CG_VIS_SCENE_HPP
