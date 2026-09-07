#pragma once

#include <Core/String.hpp>
#include <Scripting/Interface/IScriptClass.hpp>
#include <Scripting/Interface/IScriptEngine.hpp>

namespace re
{

class DotNetScriptEngine : public scripting::IScriptEngine
{
public:
	bool LoadAssembly(String const& filepath) override
	{
		// 1. Вызов C# метода через делегат для загрузки .dll
		// 2. C# сторона парсит Assembly через Reflection
		// 3. Возвращаем true, если загружено успешно
		return true;
	}

	scripting::IScriptClass* GetClass(String const& Namespace, String const& Class) override
	{
		// Поиск в кэше классов C++
		// ...
	}
};

} // namespace re
