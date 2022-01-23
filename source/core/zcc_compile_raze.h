#pragma once
#include "zcc_compile.h"

class DCoreActor;

void SetImplicitArgs(TArray<PType*>* args, TArray<uint32_t>* argflags, TArray<FName>* argnames, PContainerType* cls, uint32_t funcflags, int useflags);

class ZCCRazeCompiler : public ZCCCompiler
{
public:
	ZCCRazeCompiler(ZCC_AST &tree, DObject *outer, PSymbolTable &symbols, PNamespace *outnamespace, int lumpnum, const VersionInfo & ver)
		: ZCCCompiler(tree, outer, symbols, outnamespace, lumpnum, ver)
		{}
	int Compile() override;
protected:
	bool PrepareMetaData(PClass *type) override;
private:
	void CompileAllProperties();
	bool CompileProperties(PClass *type, TArray<ZCC_Property *> &Properties, FName prefix);
	bool CompileFlagDefs(PClass *type, TArray<ZCC_FlagDef *> &Properties, FName prefix);
	void DispatchProperty(FPropertyInfo *prop, ZCC_PropertyStmt *property, DCoreActor *defaults, Baggage &bag);
	void DispatchScriptProperty(PProperty *prop, ZCC_PropertyStmt *property, DCoreActor *defaults, Baggage &bag);
	void ProcessDefaultProperty(PClassActor *cls, ZCC_PropertyStmt *prop, Baggage &bag);
	void ProcessDefaultFlag(PClassActor *cls, ZCC_FlagStmt *flg);
	void InitDefaults() override final;
	
};


