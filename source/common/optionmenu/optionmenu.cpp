


#include "imgui.h"
#include "v_draw.h"
#include "gamecvars.h"
#include "c_buttons.h"
#include "c_bind.h"
#include "inputstate.h"

FString activeCommand;

void sendKeyForBinding(int key)
{
	Bindings.DoBind(KeyName(key), activeCommand);
	GUICapture &= ~8;
}


bool ShowOptionMenu()
{
	bool isOpen = true;
	int w = screen->GetWidth(), h = screen->GetHeight();
	double scale;
	if (w >= 1024 && h >= 768)
	{
		scale = 1;
		ImGui::SetNextWindowPos(ImVec2((w-1024)/2, (h-768)/2), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_Always);
	}
	else
	{
		// This should use a smaller font!
		scale = 640.0/1024.0;
		ImGui::SetNextWindowPos(ImVec2((w-640)/2, (h-480)/2), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Always);
	}
		

    const int window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    // Main body of the Demo window starts here.
    if (!ImGui::Begin("Customize Controls", &isOpen, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return false;
    }

	for (int i = 0; i < NUMGAMEFUNCTIONS; i++)
	{
		FString al = buttonMap.GetButtonAlias(i);
		FString cmd = "+" + al;
		if (al.IsNotEmpty())
		{
			al.Substitute('_', ' ');
			ImGui::Text(al.GetChars());
			ImGui::SameLine(300*scale);
			al = buttonMap.GetButtonName(i);
			auto binds = Bindings.GetKeysForCommand(cmd);
			al = C_NameKeys(binds.Data(), binds.Size());
			if (ImGui::Button(al, ImVec2(450 * scale, 0)))
			{
				activeCommand = cmd;
				ImGui::OpenPopup("Bind");
			}
		}
	}
	if (ImGui::BeginPopupModal("Bind", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		auto binds = Bindings.GetKeysForCommand(activeCommand);
		auto al = C_NameKeys(binds.Data(), binds.Size());

		ImGui::Text("Press 'bind' to enter binding mode or\n'delete' to clear all bindings for this action\n\nCurrently bound to this action:\n%s\n\n", al.GetChars());
		ImGui::Separator();
		if (ImGui::Button("Done", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Clear", ImVec2(120, 0))) { Bindings.UnbindACommand(activeCommand); }
		ImGui::SameLine();
		if (ImGui::Button("Bind", ImVec2(120, 0)))
		{
			// Todo: Set event handler to binding mode.
			// Wait for a bound key to arrive and add to the current command.
			GUICapture |= 8;
		}
		ImGui::EndPopup();
	}


    ImGui::End();
	return isOpen;
}