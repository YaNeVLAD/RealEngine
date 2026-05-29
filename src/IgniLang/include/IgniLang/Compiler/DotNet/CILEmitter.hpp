#pragma once

#include <Core/String.hpp>

#include <ostream>
#include <vector>

namespace igni::dotnet
{

class CILEmitter
{
public:
	explicit CILEmitter(std::ostream& out);

	void SetStream(std::ostream& out);

	void Assembly(const re::String& name) const;
	void AssemblyExtern(const re::String& name) const;
	void BeginClass(const re::String& name, const re::String& baseClass, const re::String& modifiers = "public auto ansi beforefieldinit") const;
	void EndClass() const;
	void Field(const re::String& type, const re::String& name, bool isStatic = false) const;

	void BeginMethodBody(const re::String& signature, bool isEntryPoint, int maxStack = 8) const;
	void EndMethodBody() const;
	void LocalsInit(const std::vector<re::String>& locals, const std::vector<re::String>& types) const;

	void LdLoc(int index) const;
	void StLoc(int index) const;
	void LdArg(int index) const;
	void LdArg0() const;
	void StArg(int index) const;

	void LdFld(const re::String& type, const re::String& className, const re::String& fieldName) const;
	void StFld(const re::String& type, const re::String& className, const re::String& fieldName) const;
	void LdSFld(const re::String& type, const re::String& className, const re::String& fieldName) const;
	void StSFld(const re::String& type, const re::String& className, const re::String& fieldName) const;

	void LdcI8(int64_t val) const;
	void LdcI4(int32_t val) const;
	void LdcI4_False() const;
	void LdcI4_True() const;
	void LdcR8(const re::String& val) const;
	void LdStr(const re::String& val) const;
	void LdNull() const;

	void NewObj(const re::String& signature) const;
	void NewArr(const re::String& type) const;
	void LdLen() const;
	void LdElem(const re::String& suffix) const;
	void StElem(const re::String& suffix) const;
	void CastClass(const re::String& type) const;
	void Box(const re::String& type) const;

	void Call(const re::String& signature) const;
	void CallVirtual(const re::String& signature) const;
	void Ret() const;

	void Br(const std::string& label) const;
	void BrFalse(const std::string& label) const;
	void Bge(const std::string& label) const;
	void Bgt(const std::string& label) const;
	void MarkLabel(const std::string& label) const;

	void Pop() const;
	void Dup() const;
	void ConvI4() const;
	void ConvI8() const;

	void Emit(const re::String& opcode) const;
	void Comment(const std::string_view& comment) const;
	void Raw(const std::string& rawData) const;

private:
	std::ostream* m_out;
};

} // namespace igni::dotnet