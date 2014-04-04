#pragma once

#include <functional>

#ifndef _MSC_VER
#define __thiscall
#endif

namespace vczh
{
	template<typename TResult, typename ...TArgs>
	class YBuilder
	{
	private:
		std::function<TResult(std::function<TResult(TArgs...)>, TArgs...)> partialLambda;

	public:
		YBuilder(std::function<TResult(std::function<TResult(TArgs...)>, TArgs...)> _partialLambda)
			:partialLambda(_partialLambda)
		{
		}

		TResult operator()(TArgs ...args)const
		{
			return partialLambda(
				[this](TArgs ...args)
				{
					return this->operator()(args...);
				}, args...);
		}
	};

	template<typename TMethod>
	struct PartialLambdaTypeRetriver
	{
		typedef void FunctionType;
		typedef void LambdaType;
		typedef void YBuilderType;
	};

	template<typename TClass, typename TResult, typename ...TArgs>
	struct PartialLambdaTypeRetriver<TResult(__thiscall TClass::*)(std::function<TResult(TArgs...)>, TArgs...)>
	{
		typedef TResult FunctionType(TArgs...);
		typedef TResult LambdaType(std::function<TResult(TArgs...)>, TArgs...);
		typedef YBuilder<TResult, TArgs...> YBuilderType;
	};

	template<typename TClass, typename TResult, typename ...TArgs>
	struct PartialLambdaTypeRetriver<TResult(__thiscall TClass::*)(std::function<TResult(TArgs...)>, TArgs...)const>
	{
		typedef TResult FunctionType(TArgs...);
		typedef TResult LambdaType(std::function<TResult(TArgs...)>, TArgs...);
		typedef YBuilder<TResult, TArgs...> YBuilderType;
	};

	template<typename TLambda>
	std::function<typename PartialLambdaTypeRetriver<decltype(&TLambda::operator())>::FunctionType> Y(TLambda partialLambda)
	{
		return typename PartialLambdaTypeRetriver<decltype(&TLambda::operator())>::YBuilderType(partialLambda);
	}
}
