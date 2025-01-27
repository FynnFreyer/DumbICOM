//
// Created by fynn on 20.12.22.
//

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "image_stack.hpp"
#include "dicom.hpp"


using namespace std;


image_stack::image_stack(unsigned short *data_ptr,
                         unsigned short x,
                         unsigned short y,
                         unsigned short z,
                         bool copy)
        : data_ptr(new unsigned short[fields]),
          cols(x),
          rows(y),
          image_count(z),
          fields(image_count * rows * cols) {
    init_stack(data_ptr, copy);
}

image_stack::image_stack(image_stack &from)
        : data_ptr(new unsigned short[fields]),
          cols(from.get_cols()),
          rows(from.get_rows()),
          image_count(from.get_image_count()),
          fields(cols * rows * image_count) {

    init_stack(from.data_ptr, true);
}

image_stack::image_stack(dicom &from)
        : data_ptr(new unsigned short[fields]),
          cols(from.get_cols()),
          rows(from.get_rows()),
          image_count(from.get_image_count()),
          fields(cols * rows * image_count) {

    init_stack(from.get_data_ptr(), true);
}

image_stack::~image_stack() {
    // in the end, we have to free up the data
    // TODO just deleting the pointer is probably not enough, checkout free?
    delete[] data_ptr;
}

void image_stack::copy_data(const unsigned short *ptr) {
    // C++ makes me feel like having a shotgun pointed at my crotch
    size_t data_bytes = sizeof(unsigned short) * fields;
    memcpy((void *) data_ptr, (const void *) ptr, data_bytes);
}

void image_stack::init_images() {
    // for every image, we generate a row x col matrix,
    // with the data argument pointing to the first element in our data
    for (unsigned short i; i < get_image_count(); i++) {
        unsigned short *ptr = ptr_to(0, 0, i);
        images.emplace_back(rows, cols, CV_16UC1, ptr, sizeof(unsigned short) * rows);
    }
}

void image_stack::morph_stack(unsigned short operation, unsigned short brush_size) {
    // helper for morphing every image in the stack inplace

    // generate a structuring element with the right size
    cv::Size size(brush_size, brush_size);
    // for all image_count, we do the appropriate transformation
    for (auto image: images)
        cv::morphologyEx(image, image, operation, cv::getStructuringElement(cv::MORPH_ELLIPSE, size));

    // min and max might have changed
    establish_min_max();
}

void image_stack::open_stack(unsigned short brush_size) {
    morph_stack(cv::MORPH_OPEN, brush_size);
}

void image_stack::close_stack(unsigned short brush_size) {
    morph_stack(cv::MORPH_CLOSE, brush_size);
}

void image_stack::dilate_stack(unsigned short brush_size) {
    morph_stack(cv::MORPH_DILATE, brush_size);
}

void image_stack::erode_stack(unsigned short brush_size) {
    morph_stack(cv::MORPH_ERODE, brush_size);
}

void image_stack::operator&(const image_stack& other) {
    // A & B changes A inplace, by doing an element wise and
    unsigned short* ptr = data_ptr;
    unsigned short* other_ptr = other.data_ptr;
    unsigned short* end_ptr = data_ptr + fields;

    while (ptr < end_ptr)
        *(ptr) = *(ptr++) & *(other_ptr++);

    // min and max might have changed
    establish_min_max();
    // TODO we could do optimization here,
    //  if other.min ==  0, then min becomes 0
    //  if other.max == -1, then max stays the same
    //  if min ==  0, then it will still be zero
    //  if max ==  0, then it will still be zero
}

void image_stack::operator|(const image_stack& other) {
    // A | B changes A inplace, by doing an element wise or
    unsigned short* ptr = data_ptr;
    unsigned short* other_ptr = other.data_ptr;
    unsigned short* end_ptr = data_ptr + fields;

    while (ptr < end_ptr)
        *(ptr) = *(ptr++) | *(other_ptr++);

    establish_min_max();
}

unsigned short *image_stack::ptr_to(unsigned short x, unsigned short y, unsigned short z) {
    return data_ptr + z * rows * cols + y * cols + x;
}

unsigned short image_stack::get_at(unsigned short x, unsigned short y, unsigned short z) {
    return *(ptr_to(x, y, z));
}

void image_stack::set_at(unsigned short x, unsigned short y, unsigned short z, unsigned short new_value) {
    *ptr_to(x, y, z) = new_value;

    min = (new_value < min) ? new_value : min;
    max = (new_value > max) ? new_value : max;
}

unsigned short image_stack::get_image_count() const {
    return image_count;
}

unsigned short image_stack::get_rows() const {
    return rows;
}

unsigned short image_stack::get_cols() const {
    return cols;
}

unsigned short image_stack::get_z() const {
    return get_image_count();
}

unsigned short image_stack::get_y() const {
    return get_rows();
}

unsigned short image_stack::get_x() const {
    return get_cols();
}

void image_stack::show_at(unsigned short image_index, int delay) {
    cv::imshow("OpenCV", images[image_index]);
    cv::waitKey(delay);
    cv::destroyWindow("OpenCV");
}

cv::Mat image_stack::image_at(unsigned short image_index) {
    return images[image_index];
}

void image_stack::normalize_data() {
    // feature scaling
    // x' = round(((x-min) / (max-min)) * max_possible)

    unsigned short max_possible = -1;
    size_t range = max != min ? max - min : 1;
    double scaling_factor = max_possible / range;

    for (int i = 0; i < fields; ++i)
        *(data_ptr + i) = (unsigned short) round((*(data_ptr + i) - min) * scaling_factor);

    min = 0;
    max = max != min ? -1 : 0;
}

void image_stack::normalize_pseudo_hounsfield() {
    // instead of normal feature scaling
    // we want to preserve our original values
    // in relation to the hounsfield scale
    // our "hounsfield" data is 12 bit,
    // and we want to project to 16 bit,
    // so we just multiply by 2^4 = 16

    for (int i = 0; i < fields; ++i)
        *(data_ptr + i) << 4;

    establish_min_max();
}

void image_stack::establish_min_max() {
    // find min and max in our data
    unsigned short local_min = -1;
    unsigned short local_max = 0;

    for (int i = 0; i < fields; ++i) {
        unsigned short val = *(data_ptr + i);
        local_min = (val < local_min) ? val : local_min;
        local_max = (val > local_max) ? val : local_max;
    }

    min = local_min;
    max = local_max;
}

void image_stack::init_stack(unsigned short *ptr, bool copy) {
    if (copy)
        copy_data(ptr);
    else
        data_ptr = ptr;

    establish_min_max();
    init_images();
}

void image_stack::threshold_data(unsigned short threshold) {
    for (int i = 0; i < fields; ++i)
        *(data_ptr + i) = (*(data_ptr + i) < threshold) ? 0 : -1;

    min = 0;
    max = -1;
    // TODO not necessarily true
    //  if everything is under the threshold max is 0 and
    //  vice versa for min if everything is over threshold
}

void image_stack::mask_roi(Point2D from, Point2D to) {
    Point3D from3D{from.x, from.y, 0};
    Point3D to3D{to.x, to.y, get_z()};
    mask_roi(from3D, to3D);
}

void image_stack::mask_roi(Point3D from, Point3D to) {
    // clean all points that are under from, or over to
    // this can probably be done much nicer, but it's somewhat optimized

    // you could calculate the dimension with the most "cutaway",
    // and put that in the lowest loop to maximize savings

    // iterate over images
    for (unsigned short z = 0; z < image_count; z++) {
        // if z is oob, then xy don't matter
        if (z < from.z || to.z < z) {
            // so we set this whole image zero
            for (int i = 0; i < rows; ++i)
                for (int j = 0; j < cols; ++j)
                    set_at(j, i, z, 0);

            // and go to the next image
            continue;
        }

        // iterate over rows
        for (unsigned short y = 0; y < rows; y++) {
            // if y is oob, then x doesn't matter
            if (y < from.y || to.y < y) {
                // so we set this whole row zero
                for (int i = 0; i < cols; ++i)
                    set_at(i, y, z, 0);

                // and go to the next row
                continue;
            }

            // iterate over pixels
            for (unsigned short x = 0; x < cols; x++) {
                // if x is oob, then we set it to zero
                if (x < from.x || to.x < x) {
                    set_at(x, y, z, 0);
                    continue;
                }
            }
        }
    }

    // min and max might have changed
    establish_min_max();
}

unsigned short *image_stack::get_data_ptr() {
    return data_ptr;
}

//for (unsigned short z; z < get_image_count(); z++)
//for (unsigned short y; y < get_rows(); y++)
//for (unsigned short x; x < get_cols(); x++)
