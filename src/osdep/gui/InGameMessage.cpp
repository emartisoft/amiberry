#include <stdbool.h>
#include <stdio.h>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include <guisan/gui.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "inputdevice.h"

#ifdef ANDROIDSDL
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 600
#define DIALOG_HEIGHT 200

SDL_Surface* msg_screen;
SDL_Event msg_event;
#ifdef USE_DISPMANX
#elif USE_SDL2
SDL_Texture* msg_texture;
#endif
bool msg_done = false;
gcn::Gui* msg_gui;
gcn::SDLGraphics* msg_graphics;
gcn::SDLInput* msg_input;
gcn::SDLTrueTypeFont* msg_font;

gcn::Color msg_baseCol;
gcn::Container* msg_top;
gcn::Window* wndMsg;
gcn::Button* cmdDone;
gcn::Label* txtMsg;

class DoneActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdDone)
			msg_done = 1;
	}
};

static DoneActionListener* doneActionListener;

void message_gui_halt()
{
	msg_top->remove(wndMsg);

	delete txtMsg;
	delete cmdDone;
	delete doneActionListener;
	delete wndMsg;

	delete msg_font;
	delete msg_top;

	delete msg_gui;
	delete msg_input;
	delete msg_graphics;

	if (msg_screen != nullptr)
	{
		SDL_FreeSurface(msg_screen);
		msg_screen = nullptr;
	}
#ifdef USE_DISPMANX
	//TODO
#elif USE_SDL2
	if (msg_texture != nullptr)
	{
		SDL_DestroyTexture(msg_texture);
		msg_texture = nullptr;
	}
	// Clear the screen
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
#endif
}

void message_UpdateScreen()
{
#ifdef USE_DISPMANX
	//TODO 
#elif USE_SDL2
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, msg_texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
#endif
}

void message_checkInput()
{
	//-------------------------------------------------
	// Check user input
	//-------------------------------------------------	

	int gotEvent = 0;
	while (SDL_PollEvent(&msg_event))
	{
		gotEvent = 1;
		if (msg_event.type == SDL_KEYDOWN)
		{
			switch (msg_event.key.keysym.sym)
			{
			case VK_Blue:
			case VK_Green:
			case SDLK_RETURN:
				msg_done = 1;
				break;
			default:
				break;
			}
		}
		else if (msg_event.type == SDL_JOYBUTTONDOWN)
		{
			if (gui_joystick)
			{
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button) ||
					SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].start_button) ||
					SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button))

					msg_done = 1;
			}
			break;
		}

		//-------------------------------------------------
		// Send event to gui-controls
		//-------------------------------------------------
#ifdef ANDROIDSDL
		androidsdl_event(event, msg_input);
#else
		msg_input->pushInput(msg_event);
#endif
	}
	if (gotEvent)
	{
		// Now we let the Gui object perform its logic.
		msg_gui->logic();
		// Now we let the Gui object draw itself.
		msg_gui->draw();
#ifdef USE_DISPMANX
		message_UpdateScreen();
#elif USE_SDL2
		SDL_UpdateTexture(msg_texture, nullptr, msg_screen->pixels, msg_screen->pitch);
#endif
	}
	// Finally we update the screen.
	message_UpdateScreen();
}

void message_gui_init(const char* msg)
{
#ifdef USE_DISPMANX
	// TODO
#elif USE_SDL2
	if (sdl_window == nullptr)
	{
		sdl_window = SDL_CreateWindow("Amiberry-GUI",
		                             SDL_WINDOWPOS_UNDEFINED,
		                             SDL_WINDOWPOS_UNDEFINED,
		                             0,
		                             0,
		                             SDL_WINDOW_FULLSCREEN_DESKTOP);
		check_error_sdl(sdl_window == nullptr, "Unable to create window");
	}
	// make the scaled rendering look smoother (linear scaling).
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (msg_screen == nullptr)
	{
		msg_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 32, 0, 0, 0, 0);
		check_error_sdl(msg_screen == nullptr, "Unable to create SDL surface");
	}

	if (renderer == nullptr)
	{
		renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(renderer == nullptr, "Unable to create a renderer");
		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);
	}

	if (msg_texture == nullptr)
	{
		msg_texture = SDL_CreateTexture(renderer, msg_screen->format->format, SDL_TEXTUREACCESS_STREAMING, msg_screen->w,
		                                msg_screen->h);
		check_error_sdl(msg_texture == nullptr, "Unable to create texture from Surface");
	}
	SDL_ShowCursor(SDL_ENABLE);
#endif

	msg_graphics = new gcn::SDLGraphics();
	msg_graphics->setTarget(msg_screen);
	msg_input = new gcn::SDLInput();
	msg_gui = new gcn::Gui();
	msg_gui->setGraphics(msg_graphics);
	msg_gui->setInput(msg_input);
}

void message_widgets_init(const char* msg)
{
	msg_baseCol = gcn::Color(170, 170, 170);

	msg_top = new gcn::Container();
	msg_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
	msg_top->setBaseColor(msg_baseCol);
	msg_gui->setTop(msg_top);

	TTF_Init();
	msg_font = new gcn::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
	gcn::Widget::setGlobalFont(msg_font);

	wndMsg = new gcn::Window("InGameMessage");
	wndMsg->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndMsg->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndMsg->setBaseColor(msg_baseCol);
	wndMsg->setCaption("Information");
	wndMsg->setTitleBarHeight(TITLEBAR_HEIGHT);

	doneActionListener = new DoneActionListener();

	cmdDone = new gcn::Button("Ok");
	cmdDone->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdDone->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdDone->setBaseColor(msg_baseCol);
	cmdDone->setId("Done");
	cmdDone->addActionListener(doneActionListener);

	txtMsg = new gcn::Label(msg);
	txtMsg->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);

	wndMsg->add(txtMsg, DISTANCE_BORDER, DISTANCE_BORDER);
	wndMsg->add(cmdDone);

	msg_top->add(wndMsg);
	cmdDone->requestFocus();
	wndMsg->requestModalFocus();
}

void message_gui_run()
{
	if (SDL_NumJoysticks() > 0)
	{
		gui_joystick = SDL_JoystickOpen(0);
	}

	// Prepare the screen once
	msg_gui->logic();
	msg_gui->draw();
#ifdef USE_DISPMANX
	//TODO
#elif USE_SDL2
	SDL_UpdateTexture(msg_texture, nullptr, msg_screen->pixels, msg_screen->pitch);
#endif
	message_UpdateScreen();

	while (!msg_done)
	{
		// Poll input
		message_checkInput();
		message_UpdateScreen();
	}

	if (gui_joystick)
	{
		SDL_JoystickClose(gui_joystick);
		gui_joystick = nullptr;
	}
}

void InGameMessage(const char* msg)
{
	message_gui_init(msg);
	message_widgets_init(msg);

	message_gui_run();

	message_gui_halt();
	SDL_ShowCursor(SDL_DISABLE);
}
