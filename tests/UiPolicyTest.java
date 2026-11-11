package flashlight;
public final class UiPolicyTest {
    static void check(boolean v){if(!v)throw new AssertionError();}
    public static void main(String[] args){
        check(UiPolicy.dimension(700,1000,24)==652);check(UiPolicy.dimension(300,200,10)==200);
        check(UiPolicy.dimension(10,500,24)==1);check(UiPolicy.dimension(0,500,24)==0);
        check(UiPolicy.size(300,30,100,10)==100);check(UiPolicy.size(300,500,100,10)==280);check(UiPolicy.size(50,20,100,10)==30);
        check(UiPolicy.position(800,300,-40,10)==10);check(UiPolicy.position(800,300,999,10)==490);
        check(UiPolicy.fromFraction(800,300,0f,10)==10);check(UiPolicy.fromFraction(800,300,1f,10)==490);
        check(UiPolicy.fromFraction(800,300,Float.NaN,10)==250);check(Math.abs(UiPolicy.toFraction(800,300,250,10)-.5f)<.001f);
        check(UiPolicy.toFraction(10,10,0,24)==.5f);check(UiPolicy.size(0,40,10,2)==0);
        check(UiPolicy.effectiveAmbient(false,.2f)==1f);check(UiPolicy.effectiveAmbient(true,.2f)==.2f);check(UiPolicy.effectiveAmbient(true,2f)==1f);check(UiPolicy.effectiveAmbient(true,Float.NaN)==.25f);
        check(UiPolicy.showControls(false,false));check(!UiPolicy.showControls(true,false));check(UiPolicy.showControls(true,true));check(UiPolicy.showControls(true,null));
        check(UiPolicy.isGameplay(true,false,false));check(!UiPolicy.isGameplay(true,false,true));check(!UiPolicy.isGameplay(null,false,false));check(!UiPolicy.isGameplay(true,true,false));
        System.out.println("overlay geometry and pause visibility tests passed");
    }
}
