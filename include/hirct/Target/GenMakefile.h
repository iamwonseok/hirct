#ifndef HIRCT_TARGET_GENMAKEFILE_H
#define HIRCT_TARGET_GENMAKEFILE_H

#include "hirct/Analysis/ModuleAnalyzer.h"
#include <string>

namespace hirct {

class GenMakefile {
public:
  explicit GenMakefile(const ModuleAnalyzer &analyzer);
  bool emit(const std::string &output_dir);

private:
  const ModuleAnalyzer &analyzer_;
};

} // namespace hirct

#endif // HIRCT_TARGET_GENMAKEFILE_H
