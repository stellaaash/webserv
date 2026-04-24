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

/**
 * @brief This functions checks whether a path is "under" a directory, that is, whether it is
 * contained within it or one of its subdirectories.
 *
 * @description Important: Both arguments must be standardized, absolute paths.
 * It is worth noting that the directory itself is considered to be "under" itself (as `.`).
 * Also worth noting that this function does not check for the existence of a file at `path`, and
 * will return true if the path is theorically under the directory.
 *
 * @see standardize_path()
 */
bool is_under_directory(const FilePath& path, const FilePath& directory) {
    if (directory.length() > path.length()) return false;

    for (size_t i = 0; i < directory.length(); ++i) {
        if (directory[i] != path[i]) return false;
    }
    return true;
}
