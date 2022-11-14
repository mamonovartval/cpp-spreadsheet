#pragma once

#include "FormulaLexer.h"
#include "common.h"

#include <forward_list>
#include <functional>
#include <stdexcept>

namespace ASTImpl {
class Expr;
}

class ParsingError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FormulaAST {
public:
    explicit FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr);
    FormulaAST(FormulaAST&&) = default;
    FormulaAST& operator=(FormulaAST&&) = default;
    ~FormulaAST();

    double Execute(std::function<const CellInterface* (Position pos)> ptrCell) const;
    void Print(std::ostream& out) const;
    void PrintFormula(std::ostream& out) const;

private:
    std::unique_ptr<ASTImpl::Expr> root_expr_;
    std::forward_list<Position> cells_;

    /*****NEW*************/
    class CellExpr final :public ASTImpl::Expr {
    public:
        CellExpr(std::function<const CellInterface* (Position pos)> ptrCell);
        void Print(std::ostream& out) const override;
        void DoPrintFormula(std::ostream& out, ASTImpl::ExprPrecedence /*precedence*/) const override;
        ASTImpl::ExprPrecedence GetPrecedence() const override;
        double Evaluate(std::function<const CellInterface* (Position pos)> ptrCell);
    private:
        std::function<const CellInterface* (Position pos)> ptrCell_;
    };
    /*********************/

};

FormulaAST ParseFormulaAST(std::istream& in);
FormulaAST ParseFormulaAST(const std::string& in_str);