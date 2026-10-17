/*
 * core.h 구현.
 */
#include "core.h"

namespace RD {

std::string toString(const Diagnostic &diagnostic) {
    const char *tag = diagnostic.severity == Severity::Error ? "[error] " : "[warning] ";
    return tag + diagnostic.message;
}

}
