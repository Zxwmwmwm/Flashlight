package flashlight;

/** Android-independent overlay layout and visibility rules. */
final class UiPolicy {
    private UiPolicy() {}
    static int dimension(int available,int desired,int margin) {
        if(available<=0)return 0;
        return Math.max(1,Math.min(desired,Math.max(1,available-2*Math.max(0,margin))));
    }
    static int size(int available,int desired,int minimum,int margin){
        int max=dimension(available,available,margin);
        if(max==0)return 0;
        return Math.max(Math.min(max,Math.max(1,minimum)),Math.min(max,desired));
    }
    static int position(int available,int size,int desired,int margin){
        int m=Math.max(0,Math.min(margin,Math.max(0,(available-size)/2)));
        return Math.max(m,Math.min(Math.max(m,available-size-m),desired));
    }
    static int fromFraction(int available,int size,float fraction,int margin){
        int m=Math.max(0,Math.min(margin,Math.max(0,(available-size)/2)));
        float f=Float.isNaN(fraction)?0.5f:Math.max(0f,Math.min(1f,fraction));
        return m+Math.round(Math.max(0,available-size-2*m)*f);
    }
    static float toFraction(int available,int size,int value,int margin){
        int m=Math.max(0,Math.min(margin,Math.max(0,(available-size)/2)));
        int span=Math.max(0,available-size-2*m);
        return span==0?.5f:Math.max(0f,Math.min(1f,(float)(value-m)/span));
    }
    static float effectiveAmbient(boolean darkWorld,float savedAmbient) {
        if(!darkWorld)return 1f;
        if(Float.isNaN(savedAmbient))return .25f;
        return Math.max(0f,Math.min(1f,savedAmbient));
    }
    static boolean showControls(boolean pauseOnly,Boolean pauseOpen) {
        return !pauseOnly || pauseOpen==null || pauseOpen.booleanValue();
    }
    static boolean isGameplay(Boolean hud,Boolean menu,Boolean pause) {
        return Boolean.TRUE.equals(hud)&&Boolean.FALSE.equals(menu)&&Boolean.FALSE.equals(pause);
    }
}
