#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace {
  struct IntcheckPass : public FunctionPass {
    static char ID;
    LLVMContext *C;
    Constant *logFunc;
    Type *VoidTy;
    Type *Int32Ty;

    IntcheckPass() : FunctionPass(ID) {}

    bool doInitialization(Module &M) {
      C = &(M.getContext());
      VoidTy = Type::getVoidTy(*C);
      Int32Ty = Type::getInt32Ty(*C);
      logFunc = M.getOrInsertFunction("logop", VoidTy, Int32Ty, NULL);
      return true;
    }

    bool shouldCheckOverflow(Instruction *I, int depth) {
      // TODO: implement simple dataflow analysis to see if the computed data is
      // flowing into malloc().
      bool flow = false;
      BasicBlock &B = *I->getParent();
      for(auto &II : B){
        int opc = (&II)->getOpcode();
        if(opc == 31 && dyn_cast<Instruction>((&II)->getOperand(0)) == I){
          flow = flow || shouldCheckOverflow(dyn_cast<Instruction>((&II)->getOperand(1)), ++depth);
        }
        else if((opc == 30 || opc == 37) && (&II)->getOperand(0) == I){
          flow = flow || shouldCheckOverflow(&II, ++depth);
        }
        else if(opc == 54 && (&II)->getOperand(0) == I){
//          errs()<<"Check point! : "<<II<<"\n";
          flow = flow || true;
        }
      }
/*
      Instruction* ii = (I)->getNextNode();
      if(ii == nullptr) return false;
//      errs()<<depth<<" : "<<*ii<<"\n";
      if(ii->getOpcode() == 31 && ii->getOperand(0) == I) {
        errs()<<*ii->getOperand(1)<<"\n";
//        shouldCheckOverflow(dyn_cast<Instruction>(ii->getOperand(1)), ++depth);
      }
      else if(ii->getOperand(0) == I || ii->getOperand(1) == I){
        if(isa<CallInst>(ii)){
          errs()<<"call point\n";
        }
        else{
          shouldCheckOverflow(ii, ++depth);
        }
      }
      */
      return flow;
    }

    Value* getLineNum(Instruction *I) {
      const DebugLoc *debugLoc = &I->getDebugLoc();

      if (debugLoc)
        return ConstantInt::get(Int32Ty, debugLoc->getLine());
      return ConstantInt::get(Int32Ty, -1);
    }

    virtual bool runOnFunction(Function &F) {
      bool res = false;
      for (auto &B : F) {
//        errs()<<((isa<BasicBlock>(&B))?"block---------------------------":"")<<"\n";
//        errs()<<"vvs "<<&B<<": "<<B<<"\n";
        for (auto &I : B) {
//            if(1 || isa<CallInst>(&I)) errs()<<"i:"<<(&I)->getOpcode()<<I<<"\n";
//            if((&I)->getOpcode() != 1) errs()<<"i:"<<(&I)->getOpcode()<<*((&I)->getOperand(0))<<"\n";
          if (auto *op = dyn_cast<BinaryOperator>(&I)) {
            // TODO: Implement the shouldCheckOverflow() function.
            if (!shouldCheckOverflow(&I, 0))
              continue;

            errs() << "Instrument: " << I << "\n";

            // Insert call instruction *after* `op`.
            // TODO: Pass more information including operands of computations.
            IRBuilder<> builder(op);
            builder.SetInsertPoint(&B, ++builder.GetInsertPoint());
            
            Value* args[] = {op, op->getOperand(0), op->getOperand(1), getLineNum(&I)};
            builder.CreateCall(logFunc, args);
            res |= true;
          }
        }
      }
      return res;
    }
  };
}

char IntcheckPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerIntcheckPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new IntcheckPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerIntcheckPass);
