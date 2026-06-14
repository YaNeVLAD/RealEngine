#pragma once

namespace igni::dotnet
{

inline void GlobalProcessor::ProcessAssemblies(const ast::Program* program) const
{
	for (const auto& stmt : program->statements)
	{
		if (const auto decl = dynamic_cast<const ast::Decl*>(stmt.get()))
		{
			ScanAnnotationsForAssembly(decl->annotations);
			if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(decl))
			{
				for (const auto& member : classDecl->members)
				{
					ScanAnnotationsForAssembly(member->annotations);
				}
			}
		}
	}
}

inline void GlobalProcessor::ScanAnnotationsForAssembly(const std::vector<std::unique_ptr<ast::AnnotationNode>>& annotations) const
{
	for (const auto& anno : annotations)
	{
		if (const auto strArg = ast::AnnotationUtils::GetAnnotationStringArg(anno.get()))
		{
			ExtractAssemblies(*strArg);
		}
	}
}

inline void GlobalProcessor::ExtractAssemblies(const re::String& signature) const
{
	std::size_t start = 0;
	while ((start = signature.Find('[', start)) != re::String::NPos)
	{
		const std::size_t end = signature.Find(']', start);
		if (end == std::string::npos)
		{
			break;
		}

		if (const re::String asmName = signature.Substring(start + 1, end - start - 1);
			asmName != "mscorlib" && std::ranges::find(m_gen.m_state.externAssemblies, asmName) == m_gen.m_state.externAssemblies.end())
		{
			m_gen.m_state.externAssemblies.push_back(asmName);
		}
		start = end + 1;
	}
}

inline void GlobalProcessor::ProcessGlobalFields(const ast::Program* program) const
{
	for (const auto& stmt : program->statements)
	{
		const auto varDecl = dynamic_cast<const ast::VarDecl*>(stmt.get());
		const auto valDecl = dynamic_cast<const ast::ValDecl*>(stmt.get());
		if (!varDecl && !valDecl)
		{
			continue;
		}

		const ast::Expr* init = varDecl ? varDecl->initializer.get() : valDecl->initializer.get();
		const re::String name = varDecl ? varDecl->name : valDecl->name;
		re::String cilType = "class [mscorlib]System.Object";

		if (init)
		{
			if (const auto semType = m_gen.m_state.analyzer.GetBindings().GetExpressionType(init))
			{
				cilType = m_gen.MapToCIL(semType->name);
			}
		}
		m_gen.m_state.globalVars[name] = cilType;
		m_gen.m_state.out << "  .field public static " << cilType << " '" << name << "'\n";
	}
}

inline void GlobalProcessor::ProcessGlobalConstructor(const ast::Program* program) const
{
	if (m_gen.m_state.globalVars.empty())
	{
		return;
	}

	m_gen.m_state.out << "  .method private hidebysig specialname rtspecialname static void .cctor() cil managed\n  {\n    .maxstack 8\n";
	m_gen.SetOutStream(m_gen.m_state.out);

	for (const auto& stmt : program->statements)
	{
		const auto varDecl = dynamic_cast<const ast::VarDecl*>(stmt.get());
		const auto valDecl = dynamic_cast<const ast::ValDecl*>(stmt.get());
		if (!varDecl && !valDecl)
		{
			continue;
		}

		const ast::Expr* init = varDecl ? varDecl->initializer.get() : valDecl->initializer.get();
		const re::String name = varDecl ? varDecl->name : valDecl->name;

		if (init)
		{
			init->Accept(m_gen);
			m_gen.m_state.out << "    stsfld " << m_gen.m_state.globalVars[name] << " IgniGlobalModule::" << name << "\n";
		}
	}
	m_gen.Ret();
	m_gen.EndMethodBody();
}

} // namespace igni::dotnet