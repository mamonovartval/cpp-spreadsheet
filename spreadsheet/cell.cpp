#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>

Cell::Cell(SheetInterface& sheet) : sheet_(sheet) {}
Cell::~Cell() {}

void Cell::Set(const std::string& text) {
  if (text.empty()) {
    Clear();
  } else if (text.front() == FORMULA_SIGN && text.size() > 1) {
    impl_ = std::make_unique<ImplCell::Formula>(sheet_, text);
  } else {
    impl_ = std::make_unique<ImplCell::Text>(text);
  }
}

void Cell::Clear() { impl_ = nullptr; }

std::vector<Position> Cell::GetReferencedCells() const {
  if (impl_ == nullptr) {
    return {};
  }
  return impl_->GetReferencedCells();
}

void Cell::ClearCache() {
  if (impl_ == nullptr) {
    return;
  }
  impl_->ClearCache();
}

const std::vector<Position> Cell::GetDependentCells() const {
  return dependent_cells_;
}

CellInterface::Value Cell::GetValue() const {
  if (impl_ == nullptr) {
    return {};
  }

  return impl_->GetValue();
}

std::string Cell::GetText() const {
  if (impl_ == nullptr) {
    return {};
  }
  return impl_->GetText();
}

void Cell::AddDependentCell(Position pos) { dependent_cells_.push_back(pos); }

namespace ImplCell {

Text::Text(const std::string& str) : text_(str) {}

CellInterface::Value Text::GetValue() {
  if (!text_.empty() && text_.front() == ESCAPE_SIGN) {
    return text_.substr(1);
  }
  return text_;
}

std::string Text::GetText() const { return text_; }

std::vector<Position> Text::GetReferencedCells() const { return {}; }

void Text::ClearCache() {}

Formula::Formula(const SheetInterface& sheet, const std::string& str)
    : sheet_(sheet) {
  try {
    formula_ptr_ = ParseFormula(str.substr(1));
  } catch (...) {
    throw FormulaException("ERROR FORMULA!");
  }
}

CellInterface::Value Formula::GetValue() {
  if (cache_ == std::nullopt) {
    const FormulaInterface::Value value = formula_ptr_->Evaluate(sheet_);
    if (std::holds_alternative<double>(value)) {
      cache_ = std::get<double>(value);
    } else {
      cache_ = std::get<FormulaError>(value);
    }
  }
  return cache_.value();
}

std::string Formula::GetText() const {
  return FORMULA_SIGN + formula_ptr_->GetExpression();
}

void Formula::ClearCache() { cache_ = std::nullopt; }

std::vector<Position> Formula::GetReferencedCells() const {
  return formula_ptr_->GetReferencedCells();
}

}  // namespace ImplCell

std::ostream& operator<<(std::ostream& output,
                         const CellInterface::Value& value) {
  std::visit([&](const auto& val) { output << val; }, value);
  return output;
}
