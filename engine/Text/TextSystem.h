#pragma once 

#include "Interface.h"
#include "Message.h"
#include "Precompiled.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <optional>


namespace Framework {



	struct TextStyle {
		std::string fontName;
		char border = '#';
		int paddint = 1;
	};

	struct TextBox {
		int x = 0;
		int y = 0;
		int width = 40;
		std::string text;
		TextStyle style;

		bool dirty = true;
	};

	class TextSystem : public InterfaceSystem
	{
	public:
		TextSystem();
		~TextSystem() override;


		void Initialize() override;
		void Update(float dt) override;
		void SendEngineMessage(Message* message) override;


		bool LoadFont(std::string const& logicalName, std::string const& ttfPath);
		bool SetActiveFont(std::string const& logicalName);
		std::optional<std::string> GetActiveFont() const;


		size_t AddTextBox(TextBox const& box);
		bool SetText(size_t handle, std::string const& text);
		bool MoveBox(size_t handle, int x, int y);
		bool SetWidth(size_t handle, int width);
		bool SetStyle(size_t handle, TextStyle const& style);



	};
}