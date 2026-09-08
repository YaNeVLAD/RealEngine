#pragma once

#include <Core/String.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>
#include <Scripting/CSharp/DotNetScriptInstance.hpp>
#include <Scripting/Interface/IScriptClass.hpp>

#include <memory>

namespace re
{

class DotNetScriptClass : public scripting::IScriptClass
{
	String m_namespace;
	String m_className;
	String m_fullName;

public:
	DotNetScriptClass(const String& classNamespace, const String& className)
		: m_namespace(classNamespace)
		, m_className(className)
		, m_fullName(classNamespace.Empty() ? className : classNamespace + "." + className)
	{
	}

	[[nodiscard]] const String& Name() const override
	{
		return m_className;
	}

	[[nodiscard]] const String& Namespace() const override
	{
		return m_namespace;
	}

	std::shared_ptr<scripting::IScriptInstance> Instantiate(std::uint64_t entityID) override
	{
		if (!scripting::DotNetInterop::CreateInstance)
		{
			return nullptr;
		}

		void* handle = scripting::DotNetInterop::CreateInstance(m_fullName.ToString().c_str(), entityID);
		if (!handle)
		{
			return nullptr;
		}

		return std::make_shared<DotNetScriptInstance>(handle);
	}
};

} // namespace re