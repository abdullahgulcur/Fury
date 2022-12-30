#pragma once

namespace Fury {

	class __declspec(dllexport) Component {

	private:

	public:

		Component();
		virtual ~Component();
		virtual void destroy();

	};
}