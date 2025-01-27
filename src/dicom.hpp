//
// Created by fynn on 19.12.22.
//

#ifndef ABGABE_CG_VIS_DICOM_HPP
#define ABGABE_CG_VIS_DICOM_HPP

#include <string>

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcfilefo.h>


using namespace std;


class dicom {
public:
    explicit dicom(const string& folder_path);
    ~dicom();

    unsigned short * get_data_ptr();

    unsigned short get_image_count() const;
    unsigned short get_rows() const;
    unsigned short get_cols() const;

    unsigned short get_z() const;
    unsigned short get_y() const;
    unsigned short get_x() const;
protected:
    string input;
    string meta_data;
public:
    const string &get_meta_data() const;

protected:

    unsigned short *data_ptr;
    unsigned short image_count;
    unsigned short rows;
    unsigned short cols;
};

#endif //ABGABE_CG_VIS_DICOM_HPP
