//
// Created by fynn on 19.12.22.
//

#ifndef ABGABE_CG_VIS_OPTIONS_HPP
#define ABGABE_CG_VIS_OPTIONS_HPP

#include <boost/program_options.hpp>
#include "convenience.hpp"

using namespace std;
namespace po = boost::program_options;


class options {
protected:
    int argc;
    char **argv;

    po::positional_options_description *positionals;
    po::options_description *flags_all;
    po::options_description *flags_generic;

    po::variables_map *parsed_args;

    void print_usage();

    void declare_args();
    void parse_args();
    void validate_args();

    void clean_up();
public:
    string input_path;
    unsigned short threshold;
    bool has_roi;
    Point2D roi_from;
    Point2D roi_to;
    unsigned short brush_size;

    options(int argc, char **argv);
};


#endif //ABGABE_CG_VIS_OPTIONS_HPP
