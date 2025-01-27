//
// Created by fynn on 20.12.22.
//

#ifndef ABGABE_CG_VIS_IMAGE_STACK_HPP
#define ABGABE_CG_VIS_IMAGE_STACK_HPP

#include <vector>
#include <opencv2/core.hpp>

#include "dicom.hpp"
#include "convenience.hpp"


class image_stack {
public:
    image_stack(unsigned short *data_ptr,
                unsigned short x,
                unsigned short y,
                unsigned short z,
                bool copy = true);

    image_stack(image_stack &from);
    explicit image_stack(dicom &from);
    ~image_stack();

    // elementwise masking
    void operator&(const image_stack& other);
    void operator|(const image_stack& other);

    // getter/setter
    unsigned short *get_data_ptr();
    inline unsigned short get_at(unsigned short x, unsigned short y, unsigned short z);
    inline void set_at(unsigned short x, unsigned short y, unsigned short z, unsigned short new_value);
    void show_at(unsigned short image_index, int delay = 0);
    cv::Mat image_at(unsigned short image_index);

    // stack data manipulation
    void normalize_data();
    void normalize_pseudo_hounsfield();
    void threshold_data(unsigned short threshold);

    void open_stack(unsigned short brush_size);
    void close_stack(unsigned short brush_size);
    void dilate_stack(unsigned short brush_size);
    void erode_stack(unsigned short brush_size);

    void mask_roi(Point3D from, Point3D to);
    void mask_roi(Point2D from, Point2D to);

    // meta data
    unsigned short get_image_count() const;
    unsigned short get_rows() const;
    unsigned short get_cols() const;

    unsigned short get_z() const;
    unsigned short get_y() const;
    unsigned short get_x() const;
protected:
    const unsigned short cols;
    const unsigned short rows;
    const unsigned short image_count;
    const size_t fields;

    void init_stack(unsigned short *ptr, bool copy);

    void copy_data(const unsigned short *ptr);
    unsigned short *data_ptr;

    void establish_min_max();
    unsigned short min;
    unsigned short max;

    void init_images();
    std::vector<cv::Mat> images;

    inline unsigned short * ptr_to(unsigned short x, unsigned short y, unsigned short z);
    void morph_stack(unsigned short operation, unsigned short brush_size);
};


#endif //ABGABE_CG_VIS_IMAGE_STACK_HPP
