package flashlight;
public final class UiPolicyTest {
    static void check(boolean v){if(!v)throw new AssertionError();}
    public static void main(String[] args){
        check(UiPolicy.dimension(700,1000,24)==652);check(UiPolicy.dimension(300,200,10)==200);
        check(UiPolicy.dimension(10,500,24)==1);check(UiPolicy.dimension(0,500,24)==0);
        check(UiPolicy.showControls(false,false));check(!UiPolicy.showControls(true,false));check(UiPolicy.showControls(true,true));check(UiPolicy.showControls(true,null));
        check(UiPolicy.isGameplay(true,false,false));check(!UiPolicy.isGameplay(true,false,true));check(!UiPolicy.isGameplay(null,false,false));check(!UiPolicy.isGameplay(true,true,false));
        System.out.println("overlay sizing and pause visibility tests passed");
    }
}
