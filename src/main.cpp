#include "options.hpp"
#include "dicom.hpp"
#include "image_stack.hpp"
#include "scene.hpp"


int main(int argc, char **argv) {
    options opts(argc, argv);
    dicom dcm(opts.input_path);

    image_stack unchanged(dcm.get_data_ptr(),
                          dcm.get_x(),
                          dcm.get_y(),
                          dcm.get_z(),
                          false);

    image_stack mask(unchanged);

    mask.threshold_data(opts.threshold);

    if (opts.has_roi)
        mask.mask_roi(opts.roi_from, opts.roi_to);

    mask.open_stack(opts.brush_size);
    mask.close_stack(opts.brush_size * 2);
    mask.dilate_stack(opts.brush_size * 2);

    image_stack masked(unchanged);

    masked & mask;
    masked.normalize_pseudo_hounsfield();

    scene s(masked.get_data_ptr(),
            masked.get_x(),
            masked.get_y(),
            masked.get_z(),
            dcm.get_meta_data());

    return s.render();
}
