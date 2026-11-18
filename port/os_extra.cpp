// System functions the runtime (written for Tekken Tag 2 and Smash) lacks and other titles call.
// Each answers as a console would in the situation the port is in: offline, no amiibo, no Wii Remotes,
// GamePad lying still. Where something real is left out, a line says so once.

#include "cafe/export.h"
#include "cafe/guest.h"
#include "cafe/runtime.h"
#include "cafe/sysmem.h"
#include "os/kernel.h"

#include <zlib.h>

#include <atomic>
#include <cstdio>
#include <cstring>

namespace cafe::os {
namespace {

constexpr uint32_t kClosed = 0xA0A0CE00u; // the nn::Result the runtime gives for closed online services
constexpr uint32_t kNfpNoTag = 0xA1B10400u; // as os/offline_services.cpp

void once(std::atomic<bool>& said, const char* what) {
    if (!said.exchange(true)) std::fprintf(stderr, "port: %s\n", what);
}

void no_op() {}
int32_t zero() { return 0; }
int32_t one() { return 1; }
int32_t minus_one() { return -1; }
uint32_t closed() { return kClosed; }
uint32_t no_tag() { return kNfpNoTag; }

// ------------------------------------------------------------------ nn_fp, nn_ac, nn_act: offline
int32_t ac_close_status(be<uint32_t>* status) {
    if (status) *status = 0;
    return 0;
}

// ------------------------------------------------------------------ nn_save: no other title's saves
constexpr int32_t kFsNotFound = -6;
int32_t fs_not_found() { return kFsNotFound; }

// ------------------------------------------------------------------ zlib's one-call forms
int32_t z_compress2(uint8_t* dest, be<uint32_t>* dest_len, const uint8_t* source, uint32_t source_len, int32_t level) {
    uLongf size = *dest_len;
    const int result = ::compress2(dest, &size, source, source_len, level);
    *dest_len = static_cast<uint32_t>(size);
    return result;
}
uint32_t z_compress_bound(uint32_t source_len) { return static_cast<uint32_t>(::compressBound(source_len)); }
int32_t z_uncompress(uint8_t* dest, be<uint32_t>* dest_len, const uint8_t* source, uint32_t source_len) {
    uLongf size = *dest_len;
    const int result = ::uncompress(dest, &size, source, source_len);
    *dest_len = static_cast<uint32_t>(size);
    return result;
}

// ------------------------------------------------------------------ coreinit
// No environment variables are set on a retail console.
int32_t ENVGetEnvironmentVariable(const char*, char* buffer, uint32_t size) {
    if (buffer && size) buffer[0] = 0;
    return -1;
}

// MEMAllocator {functions, heap, param1, param2} for the default heap: its two functions go through the
// title's replaceable MEMAllocFromDefaultHeapEx / MEMFreeToDefaultHeap pointers, as the console's do.
uint32_t allocator_default_alloc(PPCContext& ctx, GuestAddress, uint32_t size) { return default_heap_alloc(ctx, size, 4); }
void allocator_default_free(PPCContext& ctx, GuestAddress, uint32_t block) { default_heap_free(ctx, block); }
void thunk_allocator_alloc(PPCContext& ctx, uint8_t*) { abi::invoke<allocator_default_alloc>(ctx); }
void thunk_allocator_free(PPCContext& ctx, uint8_t*) { abi::invoke<allocator_default_free>(ctx); }
void MEMInitAllocatorForDefaultHeap(be<uint32_t>* allocator) {
    static uint32_t functions = 0;
    if (functions == 0) {
        functions = system_alloc(8, 4);
        guest<be<uint32_t>>(functions)[0] = register_host_function(thunk_allocator_alloc, "allocator: default heap alloc");
        guest<be<uint32_t>>(functions)[1] = register_host_function(thunk_allocator_free, "allocator: default heap free");
    }
    allocator[0] = functions;
    allocator[1] = 0;
    allocator[2] = 0;
    allocator[3] = 0;
}

// ------------------------------------------------------------------ gx2: the profiler measures nothing
std::atomic<bool> g_said_perf{false};
void perf_no_op() { once(g_said_perf, "GX2Perf* (the GPU profiler) measures nothing"); }
int32_t perf_no_result() { return 0; }

// ------------------------------------------------------------------ sound: effects and the GamePad's virtual surround
// The runtime's mixer has no auxiliary buses, so effect callbacks (reverb, chorus, delay) are registered and never
// run: the sound plays dry. The AXFX effects therefore only need to exist.
std::atomic<bool> g_said_aux{false}, g_said_fx{false};
int32_t ax_register_aux(uint32_t, GuestAddress, GuestAddress) {
    once(g_said_aux, "AXRegisterAuxCallback: the mixer has no effect buses, sound plays without reverb/chorus/delay");
    return 0;
}
int32_t fx_mem_size() { return 0x100; }
int32_t fx_init() {
    once(g_said_fx, "AXFX effects (reverb, chorus, delay) are not computed");
    return 1;
}
int32_t fx_true() { return 1; }
int32_t ax_drc_vs_mode(be<uint32_t>* mode) {
    if (mode) *mode = 0; // virtual surround off
    return 0;
}
uint32_t g_fx_alloc = 0, g_fx_free = 0;
void fx_set_hooks(uint32_t alloc, uint32_t free) { g_fx_alloc = alloc; g_fx_free = free; }
void fx_get_hooks(be<uint32_t>* alloc, be<uint32_t>* free) {
    if (alloc) *alloc = g_fx_alloc;
    if (free) *free = g_fx_free;
}

} // namespace
} // namespace cafe::os

using namespace cafe::os;

CAFE_EXPORT(nn_fp, IsOnline__Q2_2nn2fpFv, zero);
CAFE_EXPORT(nn_fp, IsJoinable__Q2_2nn2fpFPCQ3_2nn2fp14FriendPresenceUL, zero);
CAFE_EXPORT(nn_fp, GetBlackListAccountId__Q2_2nn2fpFPA17_cPCUiUi, closed);
CAFE_EXPORT(nn_fp, GetFriendAccountId__Q2_2nn2fpFPA17_cPCUiUi, closed);
CAFE_EXPORT(nn_fp, GetFriendRequestAccountId__Q2_2nn2fpFPA17_cPCUiUi, closed);
CAFE_EXPORT(nn_ac, GetCloseStatus__Q2_2nn2acFPQ3_2nn2ac6Status, ac_close_status);
CAFE_EXPORT(nn_act, GetHostServerSettings__Q2_2nn3actFPcT1Uc, closed);

CAFE_EXPORT(nn_nfp, GetErrorCode__Q2_2nn3nfpFRCQ2_2nn6Result, zero);
CAFE_EXPORT(nn_nfp, GetNfpRomInfo__Q2_2nn3nfpFPQ3_2nn3nfp7RomInfo, no_tag);
CAFE_EXPORT(nn_nfp, MountRom__Q2_2nn3nfpFv, no_tag);
CAFE_EXPORT(nn_nfp, SetActivateEvent__Q2_2nn3nfpFP7OSEvent, zero);   // never signalled: no tag comes
CAFE_EXPORT(nn_nfp, SetDeactivateEvent__Q2_2nn3nfpFP7OSEvent, zero);

CAFE_EXPORT(nn_save, SAVEGetStatOtherApplication, fs_not_found);
CAFE_EXPORT(nn_save, SAVEOpenFileOtherApplication, fs_not_found);

CAFE_EXPORT(nsysnet, accept, minus_one);
CAFE_EXPORT(nsysnet, listen, minus_one);

CAFE_EXPORT(padscore, KPADDisableStickCrossClamp, no_op);
CAFE_EXPORT(padscore, WPADGetBatteryLevel, zero);
CAFE_EXPORT(padscore, WPADGetSpeakerVolume, zero);
CAFE_EXPORT(padscore, WPADIsSpeakerEnabled, zero);

CAFE_EXPORT(proc_ui, ProcUIIsRunning, one);
CAFE_EXPORT(sysapp, SYSLaunchSettings, no_op);
CAFE_EXPORT(coreinit, OSIsHomeButtonMenuEnabled, zero);
CAFE_EXPORT(coreinit, ENVGetEnvironmentVariable, ENVGetEnvironmentVariable);
CAFE_EXPORT(coreinit, MEMInitAllocatorForDefaultHeap, MEMInitAllocatorForDefaultHeap);

// The GamePad's motion sensor: its calibration and correction settings are accepted; the port's GamePad lies still.
CAFE_EXPORT(vpad, VPADDisableStickCrossClamp, no_op);
CAFE_EXPORT(vpad, VPADInitGyroAccReviseParam, no_op);
CAFE_EXPORT(vpad, VPADInitGyroDirReviseParam, no_op);
CAFE_EXPORT(vpad, VPADInitGyroZeroDriftMode, no_op);
CAFE_EXPORT(vpad, VPADInitGyroZeroPlayParam, no_op);
CAFE_EXPORT(vpad, VPADSetGyroAngle, no_op);
CAFE_EXPORT(vpad, VPADSetGyroDirReviseBase, no_op);
CAFE_EXPORT(vpad, VPADSetGyroDirReviseParam, no_op);
CAFE_EXPORT(vpad, VPADSetGyroDirection, no_op);
CAFE_EXPORT(vpad, VPADSetGyroDirectionMag, no_op);
CAFE_EXPORT(vpad, VPADSetGyroMagnification, no_op);
CAFE_EXPORT(vpad, VPADSetGyroZeroPlayParam, no_op);

CAFE_EXPORT(zlib125, compress2, z_compress2);
CAFE_EXPORT(zlib125, compressBound, z_compress_bound);
CAFE_EXPORT(zlib125, uncompress, z_uncompress);

CAFE_EXPORT(gx2, GX2PerfFrameEnd, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfFrameStart, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfFree, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfGetResultByFrame, perf_no_result);
CAFE_EXPORT(gx2, GX2PerfInit, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfMetricEnable, perf_no_result);
CAFE_EXPORT(gx2, GX2PerfMetricsClear, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfPassEnd, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfPassStart, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfSetCollectionMethod, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfTagEnable, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfTagEnd, perf_no_op);
CAFE_EXPORT(gx2, GX2PerfTagStart, perf_no_op);

CAFE_EXPORT(gx2, GX2CheckSurfaceUseVsFormat, one); // every use/format pair the title asks about is drawable here
CAFE_EXPORT(gx2, GX2ResetGPU, no_op);

CAFE_EXPORT(snd_core, AXRegisterAuxCallback, ax_register_aux);
CAFE_EXPORT(snd_core, AXSetAuxReturnVolume, zero);
CAFE_EXPORT(snd_core, AXGetDRCVSMode, ax_drc_vs_mode);
CAFE_EXPORT(snd_core, AXSetDRCVSDownmixBalance, zero);
CAFE_EXPORT(snd_core, AXSetDRCVSLC, zero);
CAFE_EXPORT(snd_core, AXSetDRCVSSpeakerPosition, zero);
CAFE_EXPORT(snd_core, AXSetDRCVSSurroundDepth, zero);
CAFE_EXPORT(snd_core, AXSetDRCVSSurroundLevelGain, zero);
// The Wii Remote's speaker: no remote, no samples wanted.
CAFE_EXPORT(snd_core, AXRmtAdvancePtr, zero);
CAFE_EXPORT(snd_core, AXRmtGetSamples, zero);
CAFE_EXPORT(snd_core, AXRmtGetSamplesLeft, zero);
CAFE_EXPORT(snd_core, AXSetVoiceRmtOn, no_op);

CAFE_EXPORT(snd_user, AXFXSetHooks, fx_set_hooks);
CAFE_EXPORT(snd_user, AXFXGetHooks, fx_get_hooks);
CAFE_EXPORT(snd_user, AXFX2SetMemAllocFns, fx_set_hooks);
CAFE_EXPORT(snd_user, AXFX2GetMemAllocFns, fx_get_hooks);
CAFE_EXPORT(snd_user, AXFXMultiChChorusGetMemSize, fx_mem_size);
CAFE_EXPORT(snd_user, AXFXMultiChDelayGetMemSize, fx_mem_size);
CAFE_EXPORT(snd_user, AXFXMultiChReverbGetMemSize, fx_mem_size);
CAFE_EXPORT(snd_user, AXFXMultiChChorusInit, fx_init);
CAFE_EXPORT(snd_user, AXFXMultiChDelayInit, fx_init);
CAFE_EXPORT(snd_user, AXFXMultiChReverbInit, fx_init);
CAFE_EXPORT(snd_user, AXFXMultiChChorusShutdown, no_op);
CAFE_EXPORT(snd_user, AXFXMultiChDelayShutdown, no_op);
CAFE_EXPORT(snd_user, AXFXMultiChReverbShutdown, no_op);
CAFE_EXPORT(snd_user, AXFXMultiChChorusCallback, no_op);
CAFE_EXPORT(snd_user, AXFXMultiChDelayCallback, no_op);
CAFE_EXPORT(snd_user, AXFXMultiChReverbCallback, no_op);
CAFE_EXPORT(snd_user, AXFXMultiChChorusSettingsUpdateNoReset, fx_true);
CAFE_EXPORT(snd_user, AXFXMultiChDelaySettingsUpdateNoReset, fx_true);
CAFE_EXPORT(snd_user, AXFXMultiChReverbSettingsUpdateNoReset, fx_true);
CAFE_EXPORT(snd_user, AXFXChorusExpSettingsUpdate, fx_true);
CAFE_EXPORT(snd_user, AXFXDelayExpSettingsUpdate, fx_true);
CAFE_EXPORT(snd_user, AXFXReverbHiExpSettingsUpdate, fx_true);
CAFE_EXPORT(snd_user, AXFXReverbStdExpSettingsUpdate, fx_true);
// Per-device output settings the mixer does not have: the TV and GamePad get the plain stereo mix at full volume.
CAFE_EXPORT(snd_core, AXSetDeviceRemixMatrix, zero);
CAFE_EXPORT(snd_core, AXSetDeviceVolume, zero);
// The error viewer (the runtime prints errors and acknowledges them): settings of its look and sound.
CAFE_EXPORT(erreula, ErrEulaSetVersion__3RplFi, no_op);
CAFE_EXPORT(erreula, ErrEulaPlayAppearSE__3RplFb, no_op);
CAFE_EXPORT(erreula, ErrEulaJump__3RplFPCcUi, no_op);
