#include "rvelapch.h"
#include "InputBindings.h"
#include "sol/sol.hpp"
#include "Input/Input.h"

using namespace rv;

void LuaBindings::RegisterInputAPI(sol::state& lua)
{
    lua["KeyCode"] = lua.create_table();
    lua["MouseCode"] = lua.create_table();

#define BIND_KEY(k) lua["KeyCode"][#k] = static_cast<int>(KeyCode::k);
	BIND_KEY(Space)
	BIND_KEY(Apostrophe)
	BIND_KEY(Comma)
	BIND_KEY(Minus)
	BIND_KEY(Period)
	BIND_KEY(Slash)

	BIND_KEY(D0)
	BIND_KEY(D1)
	BIND_KEY(D2)
	BIND_KEY(D3)
	BIND_KEY(D4)
	BIND_KEY(D5)
	BIND_KEY(D6)
	BIND_KEY(D7)
	BIND_KEY(D8)
	BIND_KEY(D9)

	BIND_KEY(Semicolon)
	BIND_KEY(Equal)

	BIND_KEY(A)
	BIND_KEY(B)
	BIND_KEY(C)
	BIND_KEY(D)
	BIND_KEY(E)
	BIND_KEY(F)
	BIND_KEY(G)
	BIND_KEY(H)
	BIND_KEY(I)
	BIND_KEY(J)
	BIND_KEY(K)
	BIND_KEY(L)
	BIND_KEY(M)
	BIND_KEY(N)
	BIND_KEY(O)
	BIND_KEY(P)
	BIND_KEY(Q)
	BIND_KEY(R)
	BIND_KEY(S)
	BIND_KEY(T)
	BIND_KEY(U)
	BIND_KEY(V)
	BIND_KEY(W)
	BIND_KEY(X)
	BIND_KEY(Y)
	BIND_KEY(Z)

	BIND_KEY(LeftBracket)
	BIND_KEY(Backslash)
	BIND_KEY(RightBracket)
	BIND_KEY(GraveAccent)

	BIND_KEY(World1)
	BIND_KEY(World2)

	BIND_KEY(Escape)
	BIND_KEY(Enter)
	BIND_KEY(Tab)
	BIND_KEY(Backspace)
	BIND_KEY(Insert)
	BIND_KEY(Delete)
	BIND_KEY(Right)
	BIND_KEY(Left)
	BIND_KEY(Down)
	BIND_KEY(Up)
	BIND_KEY(PageUp)
	BIND_KEY(PageDown)
	BIND_KEY(Home)
	BIND_KEY(End)
	BIND_KEY(CapsLock)
	BIND_KEY(ScrollLock)
	BIND_KEY(NumLock)
	BIND_KEY(PrintScreen)
	BIND_KEY(Pause)
	BIND_KEY(F1)
	BIND_KEY(F2)
	BIND_KEY(F3)
	BIND_KEY(F4)
	BIND_KEY(F5)
	BIND_KEY(F6)
	BIND_KEY(F7)
	BIND_KEY(F8)
	BIND_KEY(F9)
	BIND_KEY(F10)
	BIND_KEY(F11)
	BIND_KEY(F12)
	BIND_KEY(F13)
	BIND_KEY(F14)
	BIND_KEY(F15)
	BIND_KEY(F16)
	BIND_KEY(F17)
	BIND_KEY(F18)
	BIND_KEY(F19)
	BIND_KEY(F20)
	BIND_KEY(F21)
	BIND_KEY(F22)
	BIND_KEY(F23)
	BIND_KEY(F24)
	BIND_KEY(F25)

	BIND_KEY(KP0)
	BIND_KEY(KP1)
	BIND_KEY(KP2)
	BIND_KEY(KP3)
	BIND_KEY(KP4)
	BIND_KEY(KP5)
	BIND_KEY(KP6)
	BIND_KEY(KP7)
	BIND_KEY(KP8)
	BIND_KEY(KP9)
	BIND_KEY(KPDecimal)
	BIND_KEY(KPDivide)
	BIND_KEY(KPMultiply)
	BIND_KEY(KPSubtract)
	BIND_KEY(KPAdd)
	BIND_KEY(KPEnter)
	BIND_KEY(KPEqual)

	BIND_KEY(LeftShift)
	BIND_KEY(LeftControl)
	BIND_KEY(LeftAlt)
	BIND_KEY(LeftSuper)
	BIND_KEY(RightShift)
	BIND_KEY(RightControl)
	BIND_KEY(RightAlt)
	BIND_KEY(RightSuper)
	BIND_KEY(Menu)
#undef BIND_KEY
#define BIND_MOUSE(m) lua["MouseCode"][#m] = static_cast<int>(MouseCode::m);

    BIND_MOUSE(Button0)
    BIND_MOUSE(Button1)
    BIND_MOUSE(Button2)
    BIND_MOUSE(Button3)
    BIND_MOUSE(Button4)
    BIND_MOUSE(Button5)
    BIND_MOUSE(Button6)
    BIND_MOUSE(Button7)

    BIND_MOUSE(ButtonLast)
    BIND_MOUSE(ButtonLeft)
    BIND_MOUSE(ButtonRight)
    BIND_MOUSE(ButtonMiddle)
#undef BIND_MOUSE

    lua["MouseMode"] = lua.create_table();

    lua["MouseMode"]["VISIBLE"] = static_cast<int>(Input::MouseMode::VISIBLE);
    lua["MouseMode"]["HIDDEN"] = static_cast<int>(Input::MouseMode::HIDDEN);
    lua["MouseMode"]["CAPTURED"] = static_cast<int>(Input::MouseMode::CAPTURED);


    lua["Input"] = lua.create_table();

    lua["Input"]["IsKeyPressed"] = [](int key)
        {
            return Input::IsKeyPressed(static_cast<KeyCode>(key));
        };

    lua["Input"]["IsKeyJustPressed"] = [](int key)
        {
            return Input::IsKeyJustPressed(static_cast<KeyCode>(key));
        };

    lua["Input"]["IsMouseButtonPressed"] = [](int button)
        {
            return Input::IsMouseButtonPressed(static_cast<MouseCode>(button));
        };

    lua["Input"]["GetMousePosition"] = []()
        {
            return Input::GetMousePosition();;
        };

    lua["Input"]["SetMouseMode"] = [](int mode)
        {
            Input::SetMouseMode(static_cast<Input::MouseMode>(mode));
        };
}
