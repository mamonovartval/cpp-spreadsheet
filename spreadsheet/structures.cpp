
#include <algorithm>
#include <cctype>
#include <sstream>

#include "common.h"

using namespace std;

const int LETTERS = 26;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

bool Size::operator==(Size rhs) const {
  return rows == rhs.rows && cols == rhs.cols;
}

bool Position::operator==(const Position rhs) const {
  return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
  return row < rhs.row && col < rhs.col;
}

bool Position::IsValid() const {
  return row >= 0 && row < MAX_ROWS && col >= 0 && col < MAX_COLS;
}

std::string Position::ToString() const {
  if (IsValid()) {
    string str;
    int val = col;

    while (val >= 0) {
      str.insert(str.begin(), 'A' + val % LETTERS);
      val = val / LETTERS - 1;
    }

    return str + to_string(row + 1);
  }

  return {};
}

Position Position::FromString(std::string_view str) {
  auto it =
      find_if(str.begin(), str.end(), [](char ch) { return !isupper(ch); });

  string_view alpha = str.substr(0, it - str.begin());
  string_view digits = str.substr(it - str.begin());

  if (alpha.empty() || digits.empty() || alpha.size() > MAX_POS_LETTER_COUNT ||
      !isdigit(digits[0]))
    return NONE;

  int digits_int;
  istringstream iss{string{digits}};

  if (!(iss >> digits_int) || !iss.eof()) {
    return NONE;
  }

  int col = 0;
  if (alpha.size() == 3)
    col = ((alpha[0] - 'A' + 1) * LETTERS + (alpha[1] - 'A' + 1)) * LETTERS +
          alpha[2] - 'A';
  else if (alpha.size() == 2)
    col = (alpha[0] - 'A' + 1) * LETTERS + alpha[1] - 'A';
  else if (alpha.size() == 1)
    col = alpha[0] - 'A';

  if (col >= MAX_COLS) return NONE;

  return {digits_int - 1, col};
}

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const { return category_; }

bool FormulaError::operator==(FormulaError rhs) const {
  return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
  if (category_ == Category::Div0)
    return "#DIV0!";
  else if (category_ == Category::Ref)
    return "#REF!";
  else if (category_ == Category::Value)
    return "#VALUE!";
  else
    return "#UNKNOWN ERROR!";
}
