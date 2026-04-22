#include <cassert>
#include <string>

#include "file_manager.hpp"

/**
 * @brief Based on a root, resolves a relative path.
 *
 * In effect, this follows the `relative_path` from the directory `root`.
 * Careful, depending on the relative_path, this could land you above the root, which you don't want
 * for resolving HTTP requests' targets.
 */
FilePath resolve_path(const std::string& relative_path, const FilePath& root) {
    assert(relative_path[0] != '/' && "relative_path must be relative (not start with a slash)");

    std::string unresolved_path = root + "/" + relative_path;
    std::string resolved_path = standardize_path(unresolved_path);

    return resolved_path;
}
