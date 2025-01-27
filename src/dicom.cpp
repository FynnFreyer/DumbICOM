//
// Created by fynn on 19.12.22.
//

#include <set>
#include <filesystem>

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdeftag.h>

#include "dicom.hpp"

using namespace std;
namespace fs = std::filesystem;


dicom::dicom(const string &folder_path) {
    input = folder_path;

    DcmFileFormat fileformat;

    // find DICOM files in the folder
    set<string> files;
    for (auto const &entry: fs::directory_iterator{folder_path}) {
        string file = entry.path().string();
        // if (file.ends_with(".dcm"))
        //     files.insert(file);
        files.insert(file);
    }

    image_count = files.size();

    // we expect them to be alphabetically sorted, set does that for us
    // sort(files.begin(), files.end());

    // load the first file, to get rows and cols, for initialising matrix
    string first = *(files.begin());
    if (!fileformat.loadFile(first.data()).good()) {
        cerr << "Can't read first file for sniffing size and meta data: " << first << endl;
        exit(5);
    }

    DcmDataset *ds = fileformat.getDataset();

    ds->findAndGetUint16(DCM_Columns, cols);
    ds->findAndGetUint16(DCM_Rows, rows);

    OFString name;
    OFString birth_date;
    OFString age;
    OFString sex;
    OFString study_date;

    ds->findAndGetOFString(DCM_PatientName, name);
    ds->findAndGetOFString(DCM_PatientBirthDate, birth_date);
    ds->findAndGetOFString(DCM_PatientAge, age);
    ds->findAndGetOFString(DCM_PatientSex, sex);
    ds->findAndGetOFString(DCM_StudyDate, study_date);

    string born_string = string(birth_date.data());
    born_string.insert(6, 1, '-').insert(4, 1, '-');
    if (!string(age.data()).empty())
        born_string.append(" (").append(age.data()).append(")");

    string study_string = string(study_date.data());
    study_string.insert(6, 1, '-').insert(4, 1, '-');

    meta_data.append("Name: ").append(name.data()).append("\n")
             .append("Born: ").append(born_string).append("\n")
             .append("Sex: ").append(sex.data()).append("\n")
             .append("Study Date: ").append(study_string).append("\n");

    size_t bytes_per_img = sizeof(Uint16) * rows * cols;

    // this is black magic, and I'm scared
    //Uint16 *** data_ptr = reinterpret_cast<Uint16 ***>(new Uint16[image_count * cols * rows]);
    data_ptr = new Uint16[image_count * cols * rows];

    // load the data from the files into a Mat3D
    int i = 0;
    for (string file: files) {
        if (!fileformat.loadFile(file.data()).good()) {
            cerr << "Warning: Can't read file: " << file << endl;
            // exit(1);
        }

        ds = fileformat.getDataset();

        const Uint16 *img_ptr;

        // throw on bad status
        if (!ds->findAndGetUint16Array(DCM_PixelData, img_ptr).good()) {
            cerr << "Can't read pixel data from file: " << file << endl;
            exit(6);
        }

        size_t ptr_offset = i++ * rows * cols;
        memcpy((void *) (data_ptr + ptr_offset), (const void *) img_ptr, bytes_per_img);
    }
}

dicom::~dicom() {
    // data is on the heap, so we explicitly delete it, except we don't, because we shared the pointer previously
    // delete[] data_ptr;
}

unsigned short *dicom::get_data_ptr() {
    return data_ptr;
}

unsigned short dicom::get_image_count() const {
    return image_count;
}

unsigned short dicom::get_rows() const {
    return rows;
}

unsigned short dicom::get_cols() const {
    return cols;
}

unsigned short dicom::get_z() const {
    return get_image_count();
}

unsigned short dicom::get_y() const {
    return get_rows();
}

unsigned short dicom::get_x() const {
    return get_cols();
}

const string &dicom::get_meta_data() const {
    return meta_data;
}
