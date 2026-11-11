package flashlight;

/** Small, Android-independent rules used by the controls overlay. */
final class UiPolicy {
    private UiPolicy() {}
    static int dimension(int available,int desired,int margin) {
        if(available<=0)return 0;
        return Math.max(1,Math.min(desired,Math.max(1,available-2*Math.max(0,margin))));
    }
    static boolean showControls(boolean pauseOnly,Boolean pauseOpen) {
        // If the launcher cannot report pause state, leave controls accessible.
        return !pauseOnly || pauseOpen==null || pauseOpen.booleanValue();
    }
    static boolean isGameplay(Boolean hud,Boolean menu,Boolean pause) {
        return Boolean.TRUE.equals(hud)&&Boolean.FALSE.equals(menu)&&Boolean.FALSE.equals(pause);
    }
}
