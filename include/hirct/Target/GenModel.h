#ifndef HIRCT_TARGET_GENMODEL_H
#define HIRCT_TARGET_GENMODEL_H

#include "hirct/Analysis/ModuleAnalyzer.h"
#include <string>

namespace hirct {

class GenModel {
public:
  explicit GenModel(const ModuleAnalyzer &analyzer);
  bool emit(const std::string &output_dir);

private:
  const ModuleAnalyzer &analyzer_;

  std::string cpp_type_for_width(int width) const;
  bool emit_header(const std::string &dir);
  bool emit_impl(const std::string &dir);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENMODEL_H
