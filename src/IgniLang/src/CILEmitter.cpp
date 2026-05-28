#include <IgniLang/Compiler/DotNet/CILEmitter.hpp>

#include <vector>

namespace igni::dotnet
{

CILEmitter::CILEmitter(std::ostream& out)
	: m_out(&out)
{
}

void CILEmitter::SetStream(std::ostream& out)
{
	m_out = &out;
}

void CILEmitter::Assembly(const re::String& name) const
{
	*m_out << ".assembly " << name << " { }\n";
}

void CILEmitter::AssemblyExtern(const re::String& name) const
{
	*m_out << ".assembly extern " << name << " { }\n";
}

void CILEmitter::BeginClass(const re::String& name, const re::String& baseClass) const
{
	*m_out << ".class public auto ansi beforefieldinit " << name << " extends " << baseClass << "\n{\n";
}

void CILEmitter::EndClass() const
{
	*m_out << "}\n\n";
}

void CILEmitter::Field(const re::String& type, const re::String& name, bool isStatic) const
{
	*m_out << "  .field public " << (isStatic ? "static " : "") << type << " '" << name << "'\n";
}

void CILEmitter::BeginMethodBody(const re::String& signature, bool isEntryPoint, int maxStack) const
{
	*m_out << "  " << signature << "\n  {\n";
	if (isEntryPoint)
	{
		*m_out << "    .entrypoint\n";
	}
	*m_out << "    .maxstack " << maxStack << "\n";
}

void CILEmitter::EndMethodBody() const
{
	*m_out << "  }\n\n";
}

void CILEmitter::LocalsInit(const std::vector<re::String>& locals, const std::vector<re::String>& types) const
{
	*m_out << "    .locals init (\n";
	for (size_t i = 0; i < locals.size(); ++i)
	{
		*m_out << "      [" << i << "] " << types[i] << " '" << locals[i] << "'" << (i == locals.size() - 1 ? "" : ",") << "\n";
	}
	*m_out << "    )\n";
}

void CILEmitter::LdLoc(int index) const
{
	*m_out << "    ldloc " << index << "\n";
}

void CILEmitter::StLoc(int index) const
{
	*m_out << "    stloc " << index << "\n";
}

void CILEmitter::LdArg(int index) const
{
	*m_out << "    ldarg " << index << "\n";
}

void CILEmitter::LdArg0() const
{
	*m_out << "    ldarg.0\n";
}

void CILEmitter::StArg(int index) const
{
	*m_out << "    starg " << index << "\n";
}

void CILEmitter::LdFld(const re::String& type, const re::String& className, const re::String& fieldName) const
{
	*m_out << "    ldfld " << type << " " << className << "::" << fieldName << "\n";
}

void CILEmitter::StFld(const re::String& type, const re::String& className, const re::String& fieldName) const
{
	*m_out << "    stfld " << type << " " << className << "::" << fieldName << "\n";
}

void CILEmitter::LdSFld(const re::String& type, const re::String& className, const re::String& fieldName) const
{
	*m_out << "    ldsfld " << type << " " << className << "::" << fieldName << "\n";
}

void CILEmitter::StSFld(const re::String& type, const re::String& className, const re::String& fieldName) const
{
	*m_out << "    stsfld " << type << " " << className << "::" << fieldName << "\n";
}

void CILEmitter::LdcI8(const std::int64_t val) const
{
	*m_out << "    ldc.i8 " << val << "\n";
}

void CILEmitter::LdcI4(const std::int32_t val) const
{
	*m_out << "    ldc.i4 " << val << "\n";
}

void CILEmitter::LdcI4_False() const
{
	*m_out << "    ldc.i4.0\n";
}

void CILEmitter::LdcI4_True() const
{
	*m_out << "    ldc.i4.1\n";
}

void CILEmitter::LdcR8(const re::String& val) const
{
	*m_out << "    ldc.r8 " << val << "\n";
}

void CILEmitter::LdStr(const re::String& val) const
{
	*m_out << "    ldstr " << val << "\n";
}

void CILEmitter::LdNull() const
{
	*m_out << "    ldnull\n";
}

void CILEmitter::NewObj(const re::String& signature) const
{
	*m_out << "    newobj instance void " << signature << "\n";
}

void CILEmitter::NewArr(const re::String& type) const
{
	*m_out << "    newarr " << type << "\n";
}

void CILEmitter::LdLen() const { *m_out << "    ldlen\n"; }

void CILEmitter::LdElem(const re::String& suffix) const
{
	*m_out << "    ldelem." << suffix << "\n";
}

void CILEmitter::StElem(const re::String& suffix) const
{
	*m_out << "    stelem." << suffix << "\n";
}

void CILEmitter::CastClass(const re::String& type) const
{
	*m_out << "    castclass " << type << "\n";
}

void CILEmitter::Box(const re::String& type) const
{
	*m_out << "    box " << type << "\n";
}

void CILEmitter::Call(const re::String& signature) const
{
	*m_out << "    call " << signature << "\n";
}

void CILEmitter::CallVirtual(const re::String& signature) const
{
	*m_out << "    callvirt instance " << signature << "\n";
}

void CILEmitter::Ret() const
{
	*m_out << "    ret\n";
}

void CILEmitter::Br(const std::string& label) const
{
	*m_out << "    br " << label << "\n";
}

void CILEmitter::BrFalse(const std::string& label) const
{
	*m_out << "    brfalse " << label << "\n";
}

void CILEmitter::Bge(const std::string& label) const
{
	*m_out << "    bge " << label << "\n";
}

void CILEmitter::Bgt(const std::string& label) const
{
	*m_out << "    bgt " << label << "\n";
}

void CILEmitter::MarkLabel(const std::string& label) const
{
	*m_out << label << ":\n";
}

void CILEmitter::Pop() const
{
	*m_out << "    pop\n";
}

void CILEmitter::Dup() const
{
	*m_out << "    dup\n";
}

void CILEmitter::ConvI4() const
{
	*m_out << "    conv.i4\n";
}

void CILEmitter::ConvI8() const
{
	*m_out << "    conv.i8\n";
}

void CILEmitter::Emit(const std::string_view& opcode) const
{
	*m_out << "    " << opcode << "\n";
}

void CILEmitter::Comment(const std::string_view& comment) const
{
	*m_out << "    // " << comment << "\n";
}

void CILEmitter::Raw(const std::string& rawData) const
{
	*m_out << rawData;
}

} // namespace igni::dotnet