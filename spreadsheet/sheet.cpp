
#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

#include "cell.h"
#include "common.h"

using namespace std::literals;

size_t HasherPosition::operator()(Position val) const {
  return val.col * 101 + val.row;
}

Sheet::~Sheet() {}

void Sheet::ResizedPrintOblast() {
  oblast_.rows = oblast_.cols = 0;
  for (const auto& [pos, _] : sheet_) {
    oblast_.rows = std::max(oblast_.rows, pos.row + 1);
    oblast_.cols = std::max(oblast_.cols, pos.col + 1);
  }
}

void Sheet::SetCell(Position pos, std::string text) {
  IsValidCell(pos);

  if (sheet_.count(pos)) {
    CastToCell(sheet_[pos].get())->Set(std::move(text));
  } else {
    auto new_cell = std::make_unique<Cell>(*this);
    new_cell->Set(std::move(text));
    sheet_[pos] = std::move(new_cell);
  }

  for (const auto& pos_ref_cell : sheet_[pos]->GetReferencedCells()) {
    if (!pos_ref_cell.IsValid()) {
      return;
    }

    if (this->GetCell(pos_ref_cell) == nullptr) {
      auto new_cell = std::make_unique<Cell>(*this);
      sheet_[pos_ref_cell] = std::move(new_cell);
      ResizedPrintOblast();
      return;
    }
  }

  std::set<Cell*> set_visited_cell;
  CircularDepedence(CastToCell(sheet_[pos].get()),
                    CastToCell(sheet_[pos].get()), set_visited_cell);

  for (const auto& pos_ref_cell : sheet_[pos]->GetReferencedCells()) {
    auto ptr = CastToCell(this->GetCell(pos_ref_cell));
    if (ptr != nullptr) ptr->AddDependentCell(pos);
  }

  CacheInvalidation(CastToCell(sheet_[pos].get()));
  ResizedPrintOblast();
}

void Sheet::CacheInvalidation(Cell* ptr_cell) {
  for (const auto& pos_ref_cell : ptr_cell->GetDependentCells()) {
    auto ptr = CastToCell(GetCell(pos_ref_cell));
    ptr->ClearCache();
    CacheInvalidation(ptr);
  }
}

void Sheet::IsValidCell(Position pos) const {
  if (!pos.IsValid()) {
    throw InvalidPositionException("ERROR POS!");
  }
}

Cell* Sheet::CastToCell(CellInterface* ci) { return dynamic_cast<Cell*>(ci); }

const CellInterface* Sheet::GetCell(Position pos) const {
  IsValidCell(pos);
  if (!sheet_.count(pos) || sheet_.at(pos)->GetText().empty()) {
    return nullptr;
  }
  return sheet_.at(pos).get();
}

CellInterface* Sheet::GetCell(Position pos) {
  IsValidCell(pos);
  if (!sheet_.count(pos) || sheet_.at(pos)->GetText().empty()) {
    return nullptr;
  }
  return sheet_.at(pos).get();
}

void Sheet::ClearCell(Position pos) {
  IsValidCell(pos);
  if (!sheet_.count(pos)) {
    return;
  }
  sheet_.erase(pos);
  ResizedPrintOblast();
}

Size Sheet::GetPrintableSize() const { return oblast_; }

void Sheet::Print(std::ostream& output, PrintTypeData type) const {
  for (int i = 0; i < oblast_.rows; ++i) {
    for (int j = 0; j < oblast_.cols; ++j) {
      Position pos = {i, j};
      if (type == PrintTypeData::TEXT) {
        if (sheet_.count(pos))
          output << sheet_.at(pos)->GetText()
                 << (j + 1 == oblast_.cols ? "" : "\t");
        else
          output << (j + 1 == oblast_.cols ? "" : "\t");
      } else {
        if (sheet_.count(pos))
          output << sheet_.at(pos)->GetValue()
                 << (j + 1 == oblast_.cols ? "" : "\t");
        else
          output << (j + 1 == oblast_.cols ? "" : "\t");
      }
    }
    output << '\n';
  }
}

void Sheet::PrintValues(std::ostream& output) const {
  Print(output, PrintTypeData::VALUE);
}

void Sheet::PrintTexts(std::ostream& output) const {
  Print(output, PrintTypeData::TEXT);
}

std::unique_ptr<SheetInterface> CreateSheet() {
  return std::make_unique<Sheet>();
}

void Sheet::CircularDepedence(Cell* start, Cell* current,
                              std::set<Cell*>& visited) {
  if (current) {
    for (const auto& pos_ref_cell : current->GetReferencedCells()) {
      if (!pos_ref_cell.IsValid()) continue;
      auto* ptr = CastToCell(GetCell(pos_ref_cell));
      if (visited.find(ptr) != visited.end()) {
        continue;
      }

      if (start == ptr) {
        throw CircularDependencyException("CYCLE TRUE!");
      }

      visited.insert(ptr);
      CircularDepedence(start, ptr, visited);
    }
  }
}
