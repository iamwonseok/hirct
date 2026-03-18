#ifndef HIRCT_SUPPORT_PATHUTILS_H
#define HIRCT_SUPPORT_PATHUTILS_H

#include <string>

namespace hirct {

std::string to_absolute_path(const std::string &path);

/// Recursively create directories (like `mkdir -p`).
/// Returns true on success or if the directory already exists.
bool mkdir_p(const std::string &path, mode_t mode = 0755);

} // namespace hirct

#endif // HIRCT_SUPPORT_PATHUTILS_H
