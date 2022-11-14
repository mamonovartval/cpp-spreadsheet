
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>

#include "FormulaBaseListener.h"
#include "FormulaLexer.h"
#include "FormulaParser.h"

namespace ASTImpl {

enum ExprPrecedence {
  EP_ADD,
  EP_SUB,
  EP_MUL,
  EP_DIV,
  EP_UNARY,
  EP_ATOM,
  EP_END,
};

enum PrecedenceRule {
  PR_NONE = 0b00,                // never needed
  PR_LEFT = 0b01,                // needed for a left child
  PR_RIGHT = 0b10,               // needed for a right child
  PR_BOTH = PR_LEFT | PR_RIGHT,  // needed for both children
};

constexpr PrecedenceRule PRECEDENCE_RULES[EP_END][EP_END] = {
    /* EP_ADD */ {PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
    /* EP_SUB */ {PR_RIGHT, PR_RIGHT, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
    /* EP_MUL */ {PR_BOTH, PR_BOTH, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
    /* EP_DIV */ {PR_BOTH, PR_BOTH, PR_RIGHT, PR_RIGHT, PR_NONE, PR_NONE},
    /* EP_UNARY */ {PR_BOTH, PR_BOTH, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
    /* EP_ATOM */ {PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
};

class Expr {
 public:
  virtual ~Expr() = default;
  virtual void Print(std::ostream& out) const = 0;
  virtual void DoPrintFormula(std::ostream& out,
                              ExprPrecedence precedence) const = 0;
  virtual double Evaluate(std::function<double(Position)> func) const = 0;

  virtual ExprPrecedence GetPrecedence() const = 0;

  void PrintFormula(std::ostream& out, ExprPrecedence parent_precedence,
                    bool right_child = false) const {
    auto precedence = GetPrecedence();
    auto mask = right_child ? PR_RIGHT : PR_LEFT;
    bool parens_needed = PRECEDENCE_RULES[parent_precedence][precedence] & mask;
    if (parens_needed) {
      out << '(';
    }

    DoPrintFormula(out, precedence);

    if (parens_needed) {
      out << ')';
    }
  }
};

class CellExpr final : public Expr {
 public:
  CellExpr(Position* cell_ptr) : cell_ptr_(cell_ptr) {}

  void Print(std::ostream& out) const override {
    if (!cell_ptr_->IsValid()) {
      out << FormulaError::Category::Ref;
    } else {
      out << cell_ptr_->ToString();
    }
  }

  void DoPrintFormula(std::ostream& out,
                      ExprPrecedence /* precedence */) const override {
    Print(out);
  }

  ExprPrecedence GetPrecedence() const override { return EP_ATOM; }

  double Evaluate(std::function<double(Position)> func) const override {
    return func(*cell_ptr_);
  }

 private:
  const Position* cell_ptr_;
};

namespace {
class BinaryOpExpr final : public Expr {
 public:
  enum Type : char {
    Add = '+',
    Subtract = '-',
    Multiply = '*',
    Divide = '/',
  };

 public:
  explicit BinaryOpExpr(Type type, std::unique_ptr<Expr> lhs,
                        std::unique_ptr<Expr> rhs)
      : type_(type), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  void Print(std::ostream& out) const override {
    out << '(' << static_cast<char>(type_) << ' ';
    lhs_->Print(out);
    out << ' ';
    rhs_->Print(out);
    out << ')';
  }

  void DoPrintFormula(std::ostream& out,
                      ExprPrecedence precedence) const override {
    lhs_->PrintFormula(out, precedence);
    out << static_cast<char>(type_);
    rhs_->PrintFormula(out, precedence, /* right_child = */ true);
  }

  ExprPrecedence GetPrecedence() const override {
    switch (type_) {
      case Add:
        return EP_ADD;
      case Subtract:
        return EP_SUB;
      case Multiply:
        return EP_MUL;
      case Divide:
        return EP_DIV;
      default:
        assert(false);
        return static_cast<ExprPrecedence>(INT_MAX);
    }
  }

  double Evaluate(std::function<double(Position)> func) const override {
    if (type_ == Type::Add)
      return lhs_->Evaluate(func) + rhs_->Evaluate(func);
    else if (type_ == Type::Divide) {
      if (rhs_->Evaluate(func) == 0)
        throw FormulaError(FormulaError::Category::Div0);
      return lhs_->Evaluate(func) / rhs_->Evaluate(func);
    } else if (type_ == Type::Multiply)
      return lhs_->Evaluate(func) * rhs_->Evaluate(func);
    else
      return lhs_->Evaluate(func) - rhs_->Evaluate(func);
  }

 private:
  Type type_;
  std::unique_ptr<Expr> lhs_;
  std::unique_ptr<Expr> rhs_;
};

class UnaryOpExpr final : public Expr {
 public:
  enum Type : char {
    UnaryPlus = '+',
    UnaryMinus = '-',
  };

 public:
  explicit UnaryOpExpr(Type type, std::unique_ptr<Expr> operand)
      : type_(type), operand_(std::move(operand)) {}

  void Print(std::ostream& out) const override {
    out << '(' << static_cast<char>(type_) << ' ';
    operand_->Print(out);
    out << ')';
  }

  void DoPrintFormula(std::ostream& out,
                      ExprPrecedence precedence) const override {
    out << static_cast<char>(type_);
    operand_->PrintFormula(out, precedence);
  }

  ExprPrecedence GetPrecedence() const override { return EP_UNARY; }

  double Evaluate(std::function<double(Position)> func) const override {
    if (type_ == Type::UnaryMinus) return -operand_->Evaluate(func);

    return operand_->Evaluate(func);
  }

 private:
  Type type_;
  std::unique_ptr<Expr> operand_;
};

class NumberExpr final : public Expr {
 public:
  explicit NumberExpr(double value) : value_(value) {}

  void Print(std::ostream& out) const override { out << value_; }

  void DoPrintFormula(
      std::ostream& out,
      [[maybe_unused]] ExprPrecedence precedence) const override {
    out << value_;
  }

  ExprPrecedence GetPrecedence() const override { return EP_ATOM; }

  double Evaluate(
      [[maybe_unused]] std::function<double(Position)> func) const override {
    return value_;
  }

 private:
  double value_;
};

class ParseASTListener final : public FormulaBaseListener {
 public:
  std::unique_ptr<Expr> MoveRoot() {
    assert(args_.size() == 1);
    auto root = std::move(args_.front());
    args_.clear();

    return root;
  }

  std::forward_list<Position> MoveCells() { return std::move(cells_); }

 public:
  void exitUnaryOp(FormulaParser::UnaryOpContext* ctx) override {
    assert(args_.size() >= 1);

    auto operand = std::move(args_.back());

    UnaryOpExpr::Type type;
    if (ctx->SUB()) {
      type = UnaryOpExpr::UnaryMinus;
    } else {
      assert(ctx->ADD() != nullptr);
      type = UnaryOpExpr::UnaryPlus;
    }

    auto node = std::make_unique<UnaryOpExpr>(type, std::move(operand));
    args_.back() = std::move(node);
  }

  void exitLiteral(FormulaParser::LiteralContext* ctx) override {
    double value = 0;
    auto valueStr = ctx->NUMBER()->getSymbol()->getText();
    std::istringstream in(valueStr);
    in >> value;
    if (!in) {
      throw ParsingError("Invalid number: " + valueStr);
    }

    auto node = std::make_unique<NumberExpr>(value);
    args_.push_back(std::move(node));
  }

  void exitCell(FormulaParser::CellContext* cell_context) override {
    auto str = cell_context->CELL()->getSymbol()->getText();
    auto value = Position::FromString(str);
    cells_.push_front(value);

    auto node = std::make_unique<CellExpr>(&cells_.front());
    args_.push_back(std::move(node));
  }

  void exitBinaryOp(FormulaParser::BinaryOpContext* ctx) override {
    assert(args_.size() >= 2);

    auto rhs = std::move(args_.back());
    args_.pop_back();

    auto lhs = std::move(args_.back());

    BinaryOpExpr::Type type;
    if (ctx->ADD()) {
      type = BinaryOpExpr::Add;
    } else if (ctx->SUB()) {
      type = BinaryOpExpr::Subtract;
    } else if (ctx->MUL()) {
      type = BinaryOpExpr::Multiply;
    } else {
      assert(ctx->DIV() != nullptr);
      type = BinaryOpExpr::Divide;
    }

    auto node =
        std::make_unique<BinaryOpExpr>(type, std::move(lhs), std::move(rhs));
    args_.back() = std::move(node);
  }

  void visitErrorNode(antlr4::tree::ErrorNode* node) override {
    throw ParsingError("Error when parsing: " + node->getSymbol()->getText());
  }

 private:
  std::vector<std::unique_ptr<Expr>> args_;
  std::forward_list<Position> cells_;
};

class BailErrorListener : public antlr4::BaseErrorListener {
 public:
  void syntaxError([[maybe_unused]] antlr4::Recognizer* recognizer,
                   [[maybe_unused]] antlr4::Token* offendingSymbol,
                   [[maybe_unused]] size_t line,
                   [[maybe_unused]] size_t charPositionInLine,
                   [[maybe_unused]] const std::string& msg,
                   [[maybe_unused]] std::exception_ptr e) override {
    throw ParsingError("Error when lexing: " + msg);
  }
};

}  // namespace
}  // namespace ASTImpl

FormulaAST ParseFormulaAST(std::istream& in) {
  using namespace antlr4;

  ANTLRInputStream input(in);

  FormulaLexer lexer(&input);
  ASTImpl::BailErrorListener error_listener;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&error_listener);

  CommonTokenStream tokens(&lexer);

  FormulaParser parser(&tokens);
  auto error_handler = std::make_shared<BailErrorStrategy>();
  parser.setErrorHandler(error_handler);
  parser.removeErrorListeners();

  tree::ParseTree* tree = parser.main();
  ASTImpl::ParseASTListener listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

  return FormulaAST(listener.MoveRoot(), listener.MoveCells());
}

FormulaAST ParseFormulaAST(const std::string& in_str) {
  std::istringstream in(in_str);
  return ParseFormulaAST(in);
}

void FormulaAST::Print(std::ostream& out) const { root_expr_->Print(out); }

void FormulaAST::PrintCells(std::ostream& out) const {
  for (auto cell : cells_) {
    out << cell.ToString() << ' ';
  }
}

void FormulaAST::PrintFormula(std::ostream& out) const {
  root_expr_->PrintFormula(out, ASTImpl::EP_ATOM);
}

double FormulaAST::Execute(std::function<double(Position)> func) const {
  return root_expr_->Evaluate(func);
}

FormulaAST::FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr,
                       std::forward_list<Position> cells)
    : root_expr_(std::move(root_expr)), cells_(std::move(cells)) {
  cells_.sort();
}
FormulaAST::~FormulaAST() = default;

std::forward_list<Position>& FormulaAST::GetCells() { return cells_; }

const std::forward_list<Position>& FormulaAST::GetCells() const {
  return cells_;
}

