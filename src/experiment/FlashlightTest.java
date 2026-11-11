package flashlight;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import java.lang.reflect.Field;

/** A small controls panel for an experimental, initially-disabled renderer test. */
public final class FlashlightTest {
    private static native void setEnabled(boolean enabled);
    private static native void setSetting(int id,float value);
    private static native int backendStatus();
    private static native int frameCount();
    private static final Handler MAIN=new Handler(Looper.getMainLooper());
    private static boolean active,light;
    private static Activity activity; private static FrameLayout root; private static LinearLayout panel; private static TextView status; private static Button toggle;
    private static final String[] NAMES={"Red","Green","Blue","Strength","Beam size","Softness","Lag","Horizontal position","Vertical position","Ambient brightness"};
    private static final float[] DEFAULT={1f,.93f,.72f,.62f,.40f,.55f,.32f,.50f,.50f,.25f};
    private static int dp(Activity a,float v){return (int)(v*a.getResources().getDisplayMetrics().density+.5f);}
    private static GradientDrawable bg(int color,int radius,Activity a){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(a,radius));return d;}
    private static final Runnable monitor=new Runnable(){public void run(){
        if(!active)return;
        try{Class<?> c=Class.forName("org.levimc.launcher.core.mods.inbuilt.overlay.InbuiltOverlayManager");Object m=c.getMethod("getInstance").invoke(null);if(m!=null){Field f=c.getDeclaredField("activity");f.setAccessible(true);Activity a=(Activity)f.get(m);if(a!=null&&!a.isFinishing()&&a!=activity)attach(a);}}catch(Throwable ignored){}
        if(status!=null){int s=backendStatus();status.setText(s<0?"Unsupported build or hook unavailable. See FlashlightTest log.":s==0?"Waiting for game…":s==1?"Ready to test. Turn the beam on and enter a world.":s==2?"No usable OpenGL context at the render callback.":"Rendering test frames: "+frameCount());toggle.setEnabled(s>0);}
        MAIN.postDelayed(this,800);
    }};
    public static void start(){MAIN.post(new Runnable(){public void run(){if(active)return;active=true;monitor.run();}});}
    public static void stop(){
        final java.util.concurrent.CountDownLatch done=new java.util.concurrent.CountDownLatch(1);
        Runnable task=new Runnable(){public void run(){try{active=false;light=false;MAIN.removeCallbacks(monitor);detach();}finally{done.countDown();}}};
        if(Looper.myLooper()==Looper.getMainLooper())task.run();else{MAIN.post(task);try{done.await();}catch(InterruptedException e){Thread.currentThread().interrupt();}}
    }
    private static void detach(){if(root!=null&&root.getParent() instanceof ViewGroup)((ViewGroup)root.getParent()).removeView(root);root=null;panel=null;status=null;toggle=null;activity=null;}
    private static void attach(final Activity a){
        detach();activity=a;root=new FrameLayout(a);root.setClipChildren(false);final SharedPreferences prefs=a.getSharedPreferences("flashlight_test_settings",0);
        ScrollView scroll=new ScrollView(a);scroll.setFillViewport(false);scroll.setBackground(bg(0xf51a1c25,16,a));scroll.setVisibility(View.GONE);
        FrameLayout.LayoutParams pp=new FrameLayout.LayoutParams(dp(a,320),Math.min(dp(a,490),(int)(a.getResources().getDisplayMetrics().heightPixels*.82f)),Gravity.CENTER);root.addView(scroll,pp);
        panel=new LinearLayout(a);panel.setOrientation(LinearLayout.VERTICAL);panel.setPadding(dp(a,16),dp(a,14),dp(a,16),dp(a,14));scroll.addView(panel);
        TextView title=new TextView(a);title.setText("Flashlight • renderer test");title.setTextSize(19);title.setTextColor(Color.WHITE);title.setTypeface(Typeface.DEFAULT,Typeface.BOLD);panel.addView(title);
        TextView info=new TextView(a);info.setText("Screen-space experiment; not true world lighting. Starts off. UI isolation and lag need device testing.");info.setTextSize(12);info.setTextColor(0xffc4c7d2);info.setPadding(0,dp(a,5),0,dp(a,5));panel.addView(info);
        status=new TextView(a);status.setTextColor(0xffa8c9ff);status.setTextSize(12);panel.addView(status);
        toggle=new Button(a);toggle.setText("Turn beam on");toggle.setEnabled(false);panel.addView(toggle);toggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){light=!light;setEnabled(light);toggle.setText(light?"Turn beam off":"Turn beam on");}});
        for(int i=0;i<NAMES.length;i++){
            final int id=i;float value=Math.max(0,Math.min(1,prefs.getFloat("setting"+i,DEFAULT[i])));setSetting(i,value);
            final TextView label=new TextView(a);label.setTextColor(0xffe0e2eb);label.setTextSize(12);label.setText(NAMES[i]+"  "+Math.round(value*100)+"%");panel.addView(label);
            SeekBar slider=new SeekBar(a);slider.setMax(100);slider.setProgress(Math.round(value*100));panel.addView(slider);
            slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar s,int v,boolean fromUser){float x=v/100f;setSetting(id,x);label.setText(NAMES[id]+"  "+v+"%");if(fromUser)prefs.edit().putFloat("setting"+id,x).apply();}public void onStartTrackingTouch(SeekBar s){}public void onStopTrackingTouch(SeekBar s){}});
        }
        Button close=new Button(a);close.setText("Close");panel.addView(close);close.setOnClickListener(new View.OnClickListener(){public void onClick(View v){scroll.setVisibility(View.GONE);}});
        final TextView bubble=new TextView(a);bubble.setText("FL");bubble.setTextColor(Color.WHITE);bubble.setTypeface(Typeface.DEFAULT,Typeface.BOLD);bubble.setGravity(Gravity.CENTER);bubble.setBackground(bg(0xee5544a8,14,a));
        FrameLayout.LayoutParams bp=new FrameLayout.LayoutParams(dp(a,48),dp(a,48),Gravity.RIGHT|Gravity.CENTER_VERTICAL);bp.rightMargin=dp(a,12);root.addView(bubble,bp);
        bubble.setOnClickListener(new View.OnClickListener(){public void onClick(View v){scroll.setVisibility(scroll.getVisibility()==View.VISIBLE?View.GONE:View.VISIBLE);}});
        bubble.setOnTouchListener(new View.OnTouchListener(){float ox,oy,x,y;boolean moved;public boolean onTouch(View v,MotionEvent e){switch(e.getActionMasked()){
          case MotionEvent.ACTION_DOWN:ox=e.getRawX();oy=e.getRawY();x=v.getX();y=v.getY();moved=false;return true;
          case MotionEvent.ACTION_MOVE:float dx=e.getRawX()-ox,dy=e.getRawY()-oy;if(!moved&&dx*dx+dy*dy>dp(a,8)*dp(a,8))moved=true;if(moved){v.setX(Math.max(0,Math.min(root.getWidth()-v.getWidth(),x+dx)));v.setY(Math.max(0,Math.min(root.getHeight()-v.getHeight(),y+dy)));}return true;
          case MotionEvent.ACTION_UP:if(!moved)v.performClick();return true;case MotionEvent.ACTION_CANCEL:return true;default:return true;}}});
        a.addContentView(root,new ViewGroup.LayoutParams(-1,-1));
    }
}
