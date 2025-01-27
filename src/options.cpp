//
// Created by fynn on 19.12.22.
//

#include <iostream>
#include <filesystem>

#include <boost/program_options.hpp>

#include "options.hpp"


namespace fs = filesystem;
namespace po = boost::program_options;


Point2D from_csv(std::string csv) {
    size_t commas = std::count(csv.begin(), csv.end(), ',');
    bool malformed = commas != 1;

    int x;
    int y;

    if (! malformed) {
        size_t cpos = csv.find(',');
        try {
            x = std::stoi(csv.substr(0, cpos));
            y = std::stoi(csv.substr(cpos + 1, csv.length()));
        } catch (const std::invalid_argument & e) {
            malformed = true;
        }
    }

    if (malformed) {
        std::cerr << "Malformed Point2D specification: " << csv << std::endl;
        exit(8);
    }

    return Point2D{(unsigned short) x, (unsigned short) y};
}


options::options(int argc, char **argv) {
    this->argc = argc;
    this->argv = argv;

    declare_args();
    parse_args();
    validate_args();

    clean_up();
}

void options::print_usage() {
    std::cout << "Usage: " << argv[0] << " [options] <input>\n\n";
    std::cout << *flags_generic << std::endl;
}

void options::declare_args() {
    auto *opts_desc = new po::options_description("Allowed options");

    auto *opts_desc_generic = new po::options_description("Program options");
    opts_desc_generic->add_options()
        ("help,h", "print help message and exit")
        ("threshold,t", po::value<unsigned short>(), "threshold for binarization of cleaning mask (default is 250)")
        ("brush,b", po::value<unsigned short>(), "size of brush for cleaning with morphological operations (default is 25)")
        ("lower,l", po::value<string>(), "comma separated pair of integer numbers \"<row,col>\", for defining region of interest")
        ("upper,u", po::value<string>(), "comma separated pair of integer numbers \"<row,col>\", for defining region of interest");

    po::options_description opts_desc_hidden("Hidden options");
    opts_desc_hidden.add_options()("input", po::value<vector<string>>(), "input folder");

    opts_desc->add(*opts_desc_generic).add(opts_desc_hidden);

    auto* pos_opts_desc = new po::positional_options_description;
    pos_opts_desc->add("input", -1);

    flags_all = opts_desc;
    flags_generic = opts_desc_generic;
    positionals = pos_opts_desc;
}

void options::parse_args() {
    auto* var_map = new po::variables_map;
    po::store(po::command_line_parser(argc, argv).options(*flags_all).positional(*positionals).run(), *var_map);
    po::notify(*var_map);

    parsed_args = var_map;
}

void options::validate_args() {
    if (parsed_args->count("help")) {
        print_usage();
        exit(0);
    }

    if (!parsed_args->count("input")) {
        std::cerr << "No input folder passed!\n" << std::endl;
        print_usage();
        exit(1);
    }

    vector<string> input_vector = (*parsed_args)["input"].as<vector<string>>();
    if (input_vector.size() > 1) {
        std::cerr << "Only one input folder at a time is supported!\n" << std::endl;
        print_usage();
        exit(2);
    }

    string file_path_string = input_vector[0];
    fs::path file_path(file_path_string);
    if (!fs::exists(file_path)) {
        std::cerr << "The folder " << file_path_string << " does not exist, or cannot be read!\n" << std::endl;
        print_usage();
        exit(3);
    }

    if (!fs::is_directory(file_path)) {
        std::cerr << "The input path " << file_path_string << " is not a folder!\n" << std::endl;
        print_usage();
        exit(4);
    }

    input_path = file_path_string;

    has_roi = false;
    // either a lower, upper or both points of roi were passed
    if (parsed_args->count("lower") || parsed_args->count("upper")) {
        if (!parsed_args->count("lower")) {
            std::cerr << "Warning, only upper corner for roi was passed, setting lower (0, 0)\n" << std::endl;
            roi_from = Point2D{0, 0};
            roi_to = from_csv((*parsed_args)["upper"].as<std::string>());
        } else if (!parsed_args->count("upper")) {
            std::cerr << "Warning, only lower corner for roi was passed, setting upper (MAX, MAX)\n" << std::endl;
            roi_from = from_csv((*parsed_args)["lower"].as<std::string>());
            roi_to = Point2D{(unsigned short) -1, (unsigned short) -1};
        } else {
            roi_from = from_csv((*parsed_args)["lower"].as<std::string>());
            roi_to = from_csv((*parsed_args)["upper"].as<std::string>());
        }
        has_roi = true;
    }

    threshold = 250;
    if (parsed_args->count("threshold"))
        threshold = (*parsed_args)["threshold"].as<unsigned short>();

    brush_size = 25;
    if (parsed_args->count("brush"))
        brush_size = (*parsed_args)["brush"].as<unsigned short>();
}

void options::clean_up() {
    delete flags_all;
    delete flags_generic;
    delete positionals;
    delete parsed_args;
}
