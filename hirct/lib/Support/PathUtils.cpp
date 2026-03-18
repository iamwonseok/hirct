#include "hirct/Support/PathUtils.h"
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace hirct {

bool mkdir_p(const std::string &path, mode_t mode) {
  if (path.empty())
    return false;

  struct stat st;
  if (stat(path.c_str(), &st) == 0)
    return S_ISDIR(st.st_mode);

  auto pos = path.find_last_of('/');
  if (pos != std::string::npos && pos != 0) {
    if (!mkdir_p(path.substr(0, pos), mode))
      return false;
  }

  return mkdir(path.c_str(), mode) == 0 || errno == EEXIST;
}

std::string to_absolute_path(const std::string &path) {
  if (path.empty() || path[0] == '/') {
    return path;
  }
  char cwd_buf[4096];
  if (getcwd(cwd_buf, sizeof(cwd_buf)) == nullptr) {
    return path;
  }
  return std::string(cwd_buf) + "/" + path;
}

} // namespace hirct
