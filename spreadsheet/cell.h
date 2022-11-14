#pragma once

#include <optional>
#include <variant>

#include "common.h"
#include "formula.h"

class Impl {
 public:
  virtual CellInterface::Value GetValue() = 0;
  virtual std::string GetText() const = 0;
  virtual void ClearCache() = 0;
  virtual std::vector<Position> GetReferencedCells() const = 0;
  virtual ~Impl() = default;
};

namespace ImplCell {

class Text : public Impl {
 public:
  Text(const std::string& str);
  CellInterface::Value GetValue() override;
  std::string GetText() const override;
  void ClearCache() override;
  std::vector<Position> GetReferencedCells() const override;

 private:
  std::string text_;
};

class Formula : public Impl {
 public:
  Formula(const SheetInterface& sheet, const std::string& str);
  CellInterface::Value GetValue() override;
  std::string GetText() const override;
  void ClearCache() override;
  std::vector<Position> GetReferencedCells() const override;

 private:
  const SheetInterface& sheet_;
  std::unique_ptr<FormulaInterface> formula_ptr_;
  std::optional<CellInterface::Value> cache_;
};

}  // namespace ImplCell

class Cell : public CellInterface {
 public:
  Cell(SheetInterface& sheet);
  ~Cell();

  void Set(const std::string& text);
  void Clear();
  void ClearCache();
  void AddDependentCell(Position pos);

  Value GetValue() const override;
  std::string GetText() const override;
  std::vector<Position> GetReferencedCells() const override;
  const std::vector<Position> GetDependentCells() const;

 private:
  SheetInterface& sheet_;
  std::unique_ptr<Impl> impl_ = nullptr;
  std::vector<Position> dependent_cells_;
};

std::ostream& operator<<(std::ostream& output,
                         const CellInterface::Value& value);
