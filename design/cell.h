#pragma once

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
public:
    Cell();
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    /*************NEW*************/
    std::vector<Position> GetReferencedCells() const override;
    void VerifyCyclicCell() const;
    void ClearCache();
    void ClearInherCache();

private:
    class Impl {
    public:
        virtual std::string GetText() const = 0;
        virtual Value GetValue() const = 0;
    protected:
        std::string expression_;
        Value val_;
        /*****NEW*************/
        std::optional<double> cache_; // for save cache for each cell
    };
    class EmptyImpl: public Impl {
    public:
        EmptyImpl();
        
        std::string GetText() const override;
        Value GetValue() const override;
    
    };
    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view expression);
       
        std::string GetText() const override;
        Value GetValue() const override;
   
    };
    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string_view expression);
       
        std::string GetText() const override;
        Value GetValue() const override;      
    };

    std::unique_ptr<Impl> impl_;
    /************NEW*************/
    std::vector<Position> refToCell; // reference to cells from which depend this cell
    std::vector<Position> refFromCell; // depend from cells to which reference this cell
};