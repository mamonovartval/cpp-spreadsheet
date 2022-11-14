#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

#include "FormulaAST.h"

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
  return output << "#DIV/0!";
}

namespace {
class Formula : public FormulaInterface {
 public:
  explicit Formula(std::string expression)
      : ast_(ParseFormulaAST(expression)) {}

  FormulaInterface::Value Evaluate(const SheetInterface& sheet) const override {
    std::function<double(Position)> func = [&sheet](Position pos) {
      double ans = 0.0;

      if (!pos.IsValid()) throw FormulaError(FormulaError::Category::Ref);

      if (sheet.GetCell(pos) == nullptr) return ans;

      CellInterface::Value value_cell = sheet.GetCell(pos)->GetValue();

      if (std::holds_alternative<std::string>(value_cell)) {
        try {
          ans = std::stod(std::get<std::string>(value_cell));
        } catch (...) {
          throw FormulaError(FormulaError::Category::Value);
        }
      } else if (std::holds_alternative<double>(value_cell)) {
        ans = std::get<double>(value_cell);
      } else {
        throw std::get<FormulaError>(value_cell);
      }

      return ans;
    };

    FormulaInterface::Value value_fomula;
    try {
      value_fomula = ast_.Execute(func);
    } catch (FormulaError& error) {
      value_fomula = error;
    }

    return value_fomula;
  }

  std::string GetExpression() const override {
    std::ostringstream oss;
    ast_.PrintFormula(oss);
    return oss.str();
  }

  std::vector<Position> GetReferencedCells() const override {
    return {ast_.GetCells().begin(), ast_.GetCells().end()};
  }

 private:
  FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
  return std::make_unique<Formula>(std::move(expression));
}
