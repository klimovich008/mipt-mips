/**
 * main_wrapper.cpp - wrapper for entry points of MIPT-V binaries
 * @author Pavel Kryukov
 * Copyright 2019 MIPT-V
 */

#include "main_wrapper.h"

#include <infra/config/config.h>
#include <infra/exception.h>
#include <infra/ports/module.h>
#include <iostream>

static const constexpr int EXCEPTION_EXIT_CODE = 2;
static const constexpr int NO_EXCEPTION_EXIT_CODE = 3;
static const constexpr int INVALID_OPTION_EXIT_CODE = 4;

namespace config {
    static const AliasedValue<std::string> track_file = { "o", "output", "", "Save Track data to json file "};
}

MainWrapper::MainWrapper( std::string_view desc)
    : MainWrapper( desc, std::cout, std::cerr)
{ }

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays, hicpp-avoid-c-arrays)
int MainWrapper::run( int argc, const char* argv[]) const try {
    return impl( argc, argv);
}
catch ( const config::HelpOption&) {
    out << desc << std::endl << std::endl << config::help() << std::endl;
    return 0;
}
catch ( const config::InvalidOption& e) {
    err << e.what() << std::endl
        << desc << std::endl << std::endl
        << config::help() << std::endl;
    return INVALID_OPTION_EXIT_CODE;
}
catch ( const Exception& e) {
    Module::save_track_to_file(config::track_file);
    err << e.what() << std::endl;
    return EXCEPTION_EXIT_CODE;
}
catch ( const std::exception& e) {
    Module::save_track_to_file(config::track_file);
    err << "System exception:\t\n" << e.what() << std::endl;
    return EXCEPTION_EXIT_CODE;
}
catch (...) {
    Module::save_track_to_file(config::track_file);
    err << "Unknown exception\n";
    return NO_EXCEPTION_EXIT_CODE;
}
