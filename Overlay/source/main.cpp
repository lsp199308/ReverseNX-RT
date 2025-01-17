#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header
#include "dmntcht.h"

#define PROCESS_MANAGEMENT_QLAUNCH_TID 0x0100000000001000ULL

bool def = true;
bool defChanged = false;
bool isDocked = false;
bool isDockedChanged = false;
bool PluginRunning = false;
bool closed = false;
Handle debug;
uintptr_t docked_address = 0x0;
uintptr_t def_address = 0x0;
uintptr_t MAGIC_address = 0x0;
uint64_t PID = 0;
uint32_t MAGIC = 0x0;
bool check = false;
bool dmntcht = false;
bool SaltySD = false;
bool bak = false;
bool plugin = false;
bool sysclkComm = false;
char DockedChar[32];
char SystemChar[32];
char PluginChar[36];

bool CheckPort () {
	Handle saltysd;
	for (int i = 0; i < 67; i++) {
		if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
			svcCloseHandle(saltysd);
			break;
		}
		else {
			if (i == 66) return false;
			svcSleepThread(1'000'000);
		}
	}
	for (int i = 0; i < 67; i++) {
		if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
			svcCloseHandle(saltysd);
			return true;
		}
		else svcSleepThread(1'000'000);
	}
	return false;
}

bool isServiceRunning(const char *serviceName) {	
	Handle handle;	
	SmServiceName service_name = smEncodeName(serviceName);	
	if (R_FAILED(smRegisterService(&handle, service_name, false, 1))) return true;
	else {
		svcCloseHandle(handle);	
		smUnregisterService(service_name);
		return false;
	}
}

u64 GetCurrentApplicationId()
{
	// Copied from sys-clk
    Result rc = 0;
    std::uint64_t pid = 0;
    std::uint64_t tid = 0;
    rc = pmdmntGetApplicationProcessId(&pid);

    if (rc == 0x20f)
    {
        return PROCESS_MANAGEMENT_QLAUNCH_TID;
    }

    rc = pminfoGetProgramId(&tid, pid);

    if (rc == 0x20f)
    {
        return PROCESS_MANAGEMENT_QLAUNCH_TID;
    }

    return tid;
}

class GuiTest : public tsl::Gui {
public:
	GuiTest(u8 arg1, u8 arg2, bool arg3) { }

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("ReverseNX-RT", APP_VERSION);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();
		
		list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			if (SaltySD == false) renderer->drawString("SaltyNX没有运行!", false, x, y+50, 20, renderer->a(0xF33F));
			else if (plugin == false) renderer->drawString("没有检测到ReverseNX-RT插件!", false, x, y+50, 20, renderer->a(0xF33F));
			else if (check == false) {
				if (closed == true) renderer->drawString("游戏已关闭！Overlay插件禁用！\n退出Overlay插件，然后运行游戏先！", false, x, y+20, 19, renderer->a(0xF33F));
				else renderer->drawString("游戏没有运行！Overlay插件禁用！\n退出Overlay插件，然后运行游戏先!", false, x, y+20, 19, renderer->a(0xF33F));
				renderer->drawString(PluginChar, false, x, y+60, 20, renderer->a(0xFFFF));
			}
			else if (PluginRunning == false) {
				renderer->drawString("游戏正在运行。", false, x, y+20, 20, renderer->a(0xFFFF));
				renderer->drawString("ReverseNX-RT插件没有运行!", false, x, y+40, 20, renderer->a(0xF33F));
				renderer->drawString(PluginChar, false, x, y+60, 20, renderer->a(0xFFFF));
			}
			else {
				renderer->drawString("ReverseNX-RT插件正在运行。", false, x, y+20, 20, renderer->a(0xFFFF));
				if (MAGIC == 0x16BA7E39) renderer->drawString("游戏不支持更改模式!", false, x, y+40, 18, renderer->a(0xF33F));
				else if (MAGIC != 0x06BA7E39) renderer->drawString("错误的魔法数!", false, x, y+40, 20, renderer->a(0xF33F));
				else {
					renderer->drawString(SystemChar, false, x, y+40, 20, renderer->a(0xFFFF));
					renderer->drawString(DockedChar, false, x, y+60, 20, renderer->a(0xFFFF));
				}
			}
	}), 100);

		if (MAGIC == 0x06BA7E39) {
			auto *clickableListItem = new tsl::elm::ListItem("更改系统控制");
			clickableListItem->setClickListener([](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (PluginRunning == true) {
						def = !def;
						defChanged = true;
						if (dmntcht == true) {
							dmntchtWriteCheatProcessMemory(def_address, &def, 0x1);
							dmntchtReadCheatProcessMemory(def_address, &def, 0x1);
							return true;
						}
						else if (R_SUCCEEDED(svcDebugActiveProcess(&debug, PID))) {
							svcWriteDebugProcessMemory(debug, &def, def_address, 0x1);
							svcReadDebugProcessMemory(&def, debug, def_address, 0x1);
							svcCloseHandle(debug);
							return true;
						}
					}
					else return false;
				}

				return false;
			});

			list->addItem(clickableListItem);
			
			auto *clickableListItem2 = new tsl::elm::ListItem("更改模式");
			clickableListItem2->setClickListener([](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (PluginRunning == true && def == false) {
						isDocked =! isDocked;
						isDockedChanged = true;
						if (dmntcht == true) {
							dmntchtWriteCheatProcessMemory(docked_address, &isDocked, 0x1);
							dmntchtReadCheatProcessMemory(docked_address, &isDocked, 0x1);
							return true;
						}
						else if (R_SUCCEEDED(svcDebugActiveProcess(&debug, PID))) {
							svcWriteDebugProcessMemory(debug, &isDocked, docked_address, 0x1);
							svcReadDebugProcessMemory(&isDocked, debug, docked_address, 0x1);
							svcCloseHandle(debug);
							return true;
						}
					}
					else return false;
				}
				
				return false;
			});
			list->addItem(clickableListItem2);
		}
		else if (SaltySD == true && plugin == true && check == false) {
			auto *clickableListItem = new tsl::elm::ListItem("(解除)激活插件");
			clickableListItem->setClickListener([](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (bak == false) {
						rename("sdmc:/SaltySD/plugins/ReverseNX-RT.elf", "sdmc:/SaltySD/plugins/ReverseNX-RT.elf.bak");
						bak = true;
					}
					else {
						rename("sdmc:/SaltySD/plugins/ReverseNX-RT.elf.bak", "sdmc:/SaltySD/plugins/ReverseNX-RT.elf");
						bak = false;
					}
					return true;
				}
				
				return false;
			});
			list->addItem(clickableListItem);
		}

		// Add the list to the frame for it to be drawn
		frame->setContent(list);
        
		// Return the frame to have it become the top level element of this Gui
		return frame;
	}

	// Called once every frame to update values
	virtual void update() override {
		static uint8_t i = 0;
		if (R_FAILED(pmdmntGetApplicationProcessId(&PID)) && PluginRunning == true) {
			remove("sdmc:/SaltySD/ReverseNX-RT.hex");
			PluginRunning = false;
			check = false;
			closed = true;
		}
		if (PluginRunning == true) {
			if (i > 59) {
				if (dmntcht == true) dmntchtReadCheatProcessMemory(docked_address, &isDocked, 0x1);
				else if (R_SUCCEEDED(svcDebugActiveProcess(&debug, PID))) {
					svcReadDebugProcessMemory(&isDocked, debug, docked_address, 0x1);
					svcCloseHandle(debug);
				}
				if (sysclkComm && isDockedChanged && def == false && bak == false)
				{
					uint8_t sysclkConfArr[9];
					u64 currentTID = GetCurrentApplicationId();
					if (currentTID != PROCESS_MANAGEMENT_QLAUNCH_TID)
					{
						for(int i = 0; i < 8; i++) // TID to hex
							sysclkConfArr[i] = currentTID >> 8*(7-i);
						sysclkConfArr[8] = isDocked + 1;

						FILE* sysclkConf = fopen("/config/sys-clk/ReverseNX-RT.conf", "wb+");
						fwrite(sysclkConfArr, sizeof(sysclkConfArr), 1, sysclkConf);
						fclose(sysclkConf);
					}
					isDockedChanged = false;
				}
				if (sysclkComm && defChanged && def == true && bak == false) // change from force mode to system-controlled mode
				{
					uint8_t sysclkConfArr[9];
					u64 currentTID = GetCurrentApplicationId();
					if (currentTID != PROCESS_MANAGEMENT_QLAUNCH_TID)
					{
						for(int i = 0; i < 8; i++) // TID to hex
							sysclkConfArr[i] = currentTID >> 8*(7-i);
						sysclkConfArr[8] = 3;

						FILE* sysclkConf = fopen("/config/sys-clk/ReverseNX-RT.conf", "wb+");
						fwrite(sysclkConfArr, sizeof(sysclkConfArr), 1, sysclkConf);
						fclose(sysclkConf);
					}
					defChanged = false;
				}
				i = 0;
			}
			else i++;
		}
		
		if (isDocked == true) sprintf(DockedChar, "模式：底座");
		else sprintf(DockedChar, "模式：掌机");
		
		if (def == true) sprintf(SystemChar, "由系统控制: Yes");
		else sprintf(SystemChar, "由系统控制: No");
		
		if (bak == false) sprintf(PluginChar, "ReverseNX-RT插件已激活。");
		else sprintf(PluginChar, "ReverseNX-RT插件已停用。");
	
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class OverlayTest : public tsl::Overlay {
public:
	// libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
	virtual void initServices() override {
		smInitialize();
		fsdevMountSdmc();
		pminfoInitialize();
		
		SaltySD = CheckPort();
		if (SaltySD == false) return;
		
		FILE* temp = fopen("sdmc:/SaltySD/plugins/ReverseNX-RT.elf", "r");
		if (temp != NULL) {
			fclose(temp);
			plugin = true;
			sprintf(PluginChar, "ReverseNX-RT插件已激活。");
		}
		else {
			temp = fopen("sdmc:/SaltySD/plugins/ReverseNX-RT.elf.bak", "r");
			if (temp != NULL) {
				fclose(temp);
				plugin = true;
				bak = true;
				sprintf(PluginChar, "ReverseNX-RT插件已停用。");
			}
			else return;
		}
		
		// Check if sys-clk exist
		temp = fopen("/atmosphere/contents/00FF0000636C6BFF/flags/boot2.flag", "r");
		if (temp != NULL)
		{
			sysclkComm = true;
			fclose(temp);
		}


		if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) remove("sdmc:/SaltySD/ReverseNX-RT.hex");
		else {
			check = true;
			svcSleepThread(1'000'000'000);
			FILE* offset = fopen("sdmc:/SaltySD/ReverseNX-RT.hex", "rb");
			if (offset != NULL) {
				fread(&docked_address, 0x5, 1, offset);
				fread(&def_address, 0x5, 1, offset);
				fread(&MAGIC_address, 0x5, 1, offset);
				fclose(offset);
				
				dmntcht = isServiceRunning("dmnt:cht");
				
				if (dmntcht == true) {
					if (R_SUCCEEDED(dmntchtInitialize())) {
						bool out = false;
						dmntchtHasCheatProcess(&out);
						if (out == false) dmntchtForceOpenCheatProcess();
						dmntchtReadCheatProcessMemory(docked_address, &isDocked, 0x1);
						dmntchtReadCheatProcessMemory(def_address, &def, 0x1);
						dmntchtReadCheatProcessMemory(MAGIC_address, &MAGIC, 0x4);
						PluginRunning = true;
					}
					else dmntcht = false;
				}

				if (dmntcht == false) {
					svcSleepThread(1'000'000'000);
					if (R_SUCCEEDED(svcDebugActiveProcess(&debug, PID))) {
						svcReadDebugProcessMemory(&isDocked, debug, docked_address, 0x1);
						svcReadDebugProcessMemory(&def, debug, def_address, 0x1);
						svcReadDebugProcessMemory(&MAGIC, debug, MAGIC_address, 0x4);
						svcCloseHandle(debug);
						PluginRunning = true;
					}
				}
			}
		}
		
		if (isDocked == true) sprintf(DockedChar, "模式：底座");
		else sprintf(DockedChar, "模式：掌机");
		
		if (def == true) sprintf(SystemChar, "由系统控制: Yes");
		else sprintf(SystemChar, "由系统控制: No");
	
	}  // Called at the start to initialize all services necessary for this Overlay
	
	virtual void exitServices() override {
		dmntchtExit();
		if (dmntcht == false) svcCloseHandle(debug);
		pminfoExit();
		fsdevUnmountDevice("sdmc");
		smExit();
	}  // Callet at the end to clean up all services previously initialized

	virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
	
	virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

	virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
		return initially<GuiTest>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
	}
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
