#pragma once

#include <functional>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>

#include "cell.h"
#include "common.h"

enum class PrintTypeData { TEXT, VALUE };

struct HasherPosition {
  size_t operator()(Position val) const;
};

class Sheet : public SheetInterface {
 public:
  ~Sheet();
  void SetCell(Position pos, std::string text) override;
  const CellInterface* GetCell(Position pos) const override;
  CellInterface* GetCell(Position pos) override;
  void ClearCell(Position pos) override;
  Size GetPrintableSize() const override;
  void Print(std::ostream& output, PrintTypeData type) const;
  void PrintValues(std::ostream& output) const override;
  void PrintTexts(std::ostream& output) const override;

 private:
  void ResizedPrintOblast();
  void CircularDepedence(Cell* start, Cell* current, std::set<Cell*>& visited);
  void CacheInvalidation(Cell* cell_ptr);
  void IsValidCell(Position pos) const;
  Cell* CastToCell(CellInterface* ci);

 private:
  std::unordered_map<Position, std::unique_ptr<CellInterface>, HasherPosition>
      sheet_;
  Size oblast_ = {0, 0};
};
