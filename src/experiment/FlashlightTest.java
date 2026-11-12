package flashlight;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/** Experimental controls. World rendering and pause detection are separate. */
public final class FlashlightTest {
    private static native void setEnabled(boolean enabled);
    private static native void setSetting(int id,float value);
    private static native void setWorldVisible(boolean visible);
    private static native int backendStatus();
    private static native int frameCount();
    private static final Handler MAIN=new Handler(Looper.getMainLooper());
    private static final int ACCENT=0xffbba8ff,TEXT=0xfff4f3ff,MUTED=0xffabaec2;
    private static final String[] NAMES={"Red","Green","Blue","Strength","Beam size","Softness","Follow delay","Beam horizontal position","Beam vertical position","Dark-world brightness"};
    private static final float[] DEFAULT={1f,.93f,.72f,.62f,.40f,.55f,.32f,.50f,.50f,.25f};
    private static boolean active,light,pauseOnly,paused,pauseKnown,darkWorld,skyMask,visibleCone;private static int skyLook;
    private static Activity activity;private static OverlayRoot root;private static View panel;private static TextView bubble,status,toggle,pauseToggle,darkToggle,skyShaftToggle,skyHaloToggle;private static SeekBar ambientSlider;
    private static SharedPreferences prefs;private static Method pauseMethod,hudMethod,menuMethod;private static boolean inputLookupDone;
    private static int dp(Context a,float v){return (int)(v*a.getResources().getDisplayMetrics().density+.5f);}
    private static GradientDrawable bg(int color,int radius,Context a){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(a,radius));return d;}
    private static TextView text(Activity a,String value,int size,int color,boolean bold){TextView t=new TextView(a);t.setText(value);t.setTextColor(color);t.setTextSize(size);if(bold)t.setTypeface(Typeface.DEFAULT,Typeface.BOLD);return t;}
    private static TextView action(Activity a,String value,int color){TextView t=text(a,value,13,TEXT,true);t.setGravity(Gravity.CENTER);t.setBackground(bg(color,10,a));t.setPadding(dp(a,12),dp(a,10),dp(a,12),dp(a,10));t.setClickable(true);return t;}
    private static Boolean invoke(Method m){try{return m==null?null:(Boolean)m.invoke(null);}catch(Throwable ignored){return null;}}
    private static void queryScreen(){
        if(!inputLookupDone){inputLookupDone=true;try{Class<?> c=Class.forName("org.levimc.launcher.preloader.PreloaderInput");pauseMethod=c.getMethod("nativeIsPauseMenuOpen");hudMethod=c.getMethod("nativeIsHudScreenOpen");menuMethod=c.getMethod("nativeIsShowingMenu");}catch(Throwable t){Log.w("FlashlightTest","Launcher screen-state API unavailable");}}
        Boolean p=invoke(pauseMethod),h=invoke(hudMethod),m=invoke(menuMethod);pauseKnown=p!=null;paused=pauseKnown&&p.booleanValue();
        // Never treat unknown launcher state as confirmed gameplay.
        setWorldVisible(UiPolicy.isGameplay(h,m,p));
    }
    private static void updateVisibility(){
        boolean visible=UiPolicy.showControls(pauseOnly,pauseKnown?Boolean.valueOf(paused):null);
        if(bubble!=null)bubble.setVisibility(visible?View.VISIBLE:View.GONE);
        if(!visible&&panel!=null)panel.setVisibility(View.GONE);
        if(pauseToggle!=null)pauseToggle.setText("Pause-only controls   "+(pauseOnly?"ON":"OFF")+(!pauseKnown?"  •  state unavailable":""));
    }
    private static final Runnable monitor=new Runnable(){public void run(){
        if(!active)return;
        try{Class<?> c=Class.forName("org.levimc.launcher.core.mods.inbuilt.overlay.InbuiltOverlayManager");Object m=c.getMethod("getInstance").invoke(null);if(m!=null){Field f=c.getDeclaredField("activity");f.setAccessible(true);Activity a=(Activity)f.get(m);if(a!=null&&!a.isFinishing()&&a!=activity)attach(a);}}catch(Throwable ignored){}
        queryScreen();updateVisibility();
        if(status!=null){int s=backendStatus();status.setText(s<0?"Unavailable • check FlashlightTest log":s==0?"Connecting to Minecraft…":s==1?"Ready • enable beam in a world":s==2?"Waiting for a compatible world pass":s==4?"Rendering • sky depth unavailable":"Rendering • "+frameCount()+" frames");if(toggle!=null)toggle.setEnabled(s>0);}
        MAIN.postDelayed(this,200);
    }};
    public static void start(){MAIN.post(new Runnable(){public void run(){if(active)return;active=true;monitor.run();}});}
    public static void stop(){final java.util.concurrent.CountDownLatch done=new java.util.concurrent.CountDownLatch(1);Runnable task=new Runnable(){public void run(){try{active=false;light=false;MAIN.removeCallbacks(monitor);detach();}finally{done.countDown();}}};if(Looper.myLooper()==Looper.getMainLooper())task.run();else{MAIN.post(task);try{done.await();}catch(InterruptedException e){Thread.currentThread().interrupt();}}}
    private static void detach(){if(root!=null&&root.getParent() instanceof ViewGroup)((ViewGroup)root.getParent()).removeView(root);root=null;panel=null;bubble=null;status=null;toggle=null;pauseToggle=null;darkToggle=null;skyShaftToggle=null;skyHaloToggle=null;ambientSlider=null;activity=null;prefs=null;}
    private static final class OverlayRoot extends FrameLayout {
        View sheet; final SharedPreferences positionPrefs; final int margin,minWidth,minHeight;
        OverlayRoot(Context c,SharedPreferences p){super(c);positionPrefs=p;margin=dp(c,12);minWidth=dp(c,230);minHeight=dp(c,160);setClipChildren(true);setClipToPadding(true);}
        protected void onSizeChanged(int w,int h,int oldw,int oldh){super.onSizeChanged(w,h,oldw,oldh);fit(w,h);}
        void fit(int w,int h){
            if(sheet==null||w<=0||h<=0)return;
            int wantedW=positionPrefs.contains("sheetWidth")?Math.round(w*positionPrefs.getFloat("sheetWidth",.5f)):dp(getContext(),370);
            int wantedH=positionPrefs.contains("sheetHeight")?Math.round(h*positionPrefs.getFloat("sheetHeight",.7f)):dp(getContext(),500);
            int sw=UiPolicy.size(w,wantedW,minWidth,margin),sh=UiPolicy.size(h,wantedH,minHeight,margin);
            int x=UiPolicy.fromFraction(w,sw,positionPrefs.getFloat("sheetX",.5f),margin),y=UiPolicy.fromFraction(h,sh,positionPrefs.getFloat("sheetY",.5f),margin);
            apply(sw,sh,x,y);
        }
        void apply(int width,int height,int x,int y){
            int w=getWidth(),h=getHeight();if(sheet==null||w<=0||h<=0)return;
            FrameLayout.LayoutParams lp=(FrameLayout.LayoutParams)sheet.getLayoutParams();
            lp.width=UiPolicy.size(w,width,minWidth,margin);lp.height=UiPolicy.size(h,height,minHeight,margin);
            lp.leftMargin=UiPolicy.position(w,lp.width,x,margin);lp.topMargin=UiPolicy.position(h,lp.height,y,margin);
            lp.rightMargin=0;lp.bottomMargin=0;lp.gravity=Gravity.LEFT|Gravity.TOP;sheet.setLayoutParams(lp);
        }
        void save(){
            if(sheet==null||getWidth()<=0||getHeight()<=0)return;FrameLayout.LayoutParams lp=(FrameLayout.LayoutParams)sheet.getLayoutParams();
            positionPrefs.edit().putFloat("sheetWidth",(float)lp.width/getWidth()).putFloat("sheetHeight",(float)lp.height/getHeight())
                .putFloat("sheetX",UiPolicy.toFraction(getWidth(),lp.width,lp.leftMargin,margin))
                .putFloat("sheetY",UiPolicy.toFraction(getHeight(),lp.height,lp.topMargin,margin)).apply();
        }
    }
    private static void updateDarkWorld(){
        if(darkToggle!=null)darkToggle.setText("Dark world   "+(darkWorld?"ON":"OFF"));
        if(ambientSlider!=null){ambientSlider.setEnabled(darkWorld);ambientSlider.setAlpha(darkWorld?1f:.5f);}
        float value=prefs==null?.25f:prefs.getFloat("setting9",.25f);
        setSetting(9,UiPolicy.effectiveAmbient(darkWorld,value));
    }
    private static void setSkyLook(int mode){
        skyLook=Math.max(0,Math.min(2,mode));if(prefs!=null)prefs.edit().putInt("skyLook",skyLook).apply();setSetting(14,skyLook/2f);
        if(skyShaftToggle!=null)skyShaftToggle.setText("Sky shaft only   "+(skyLook==1?"ON":"OFF"));
        if(skyHaloToggle!=null)skyHaloToggle.setText("Sky soft halo   "+(skyLook==2?"ON":"OFF"));
    }
    private static void attach(final Activity a){
        detach();activity=a;prefs=a.getSharedPreferences("flashlight_test_settings",0);pauseOnly=prefs.getBoolean("pauseOnly",false);darkWorld=prefs.getBoolean("darkWorld",false);skyMask=prefs.getBoolean("skyMask",true);visibleCone=prefs.getBoolean("visibleCone",false);skyLook=Math.max(0,Math.min(2,prefs.getInt("skyLook",0)));setSetting(10,skyMask?1f:0f);setSetting(11,visibleCone?1f:0f);setSetting(14,skyLook/2f);root=new OverlayRoot(a,prefs);
        final LinearLayout sheet=new LinearLayout(a);panel=sheet;root.sheet=sheet;sheet.setOrientation(LinearLayout.VERTICAL);sheet.setBackground(bg(0xf51b1c29,18,a));sheet.setClipToOutline(true);sheet.setVisibility(View.GONE);
        FrameLayout.LayoutParams pp=new FrameLayout.LayoutParams(dp(a,320),dp(a,320),Gravity.LEFT|Gravity.TOP);root.addView(sheet,pp);
        LinearLayout header=new LinearLayout(a);header.setOrientation(LinearLayout.HORIZONTAL);header.setGravity(Gravity.CENTER_VERTICAL);header.setPadding(dp(a,16),dp(a,12),dp(a,12),dp(a,8));sheet.addView(header);
        LinearLayout titles=new LinearLayout(a);titles.setOrientation(LinearLayout.VERTICAL);header.addView(titles,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WRAP_CONTENT,1));
        titles.addView(text(a,"FLASHLIGHT  ⋮⋮",16,TEXT,true));status=text(a,"Connecting…",11,MUTED,false);titles.addView(status);
        final OverlayRoot host=root;
        titles.setContentDescription("Drag to move the Flashlight panel");
        titles.setOnTouchListener(new View.OnTouchListener(){float downX,downY;int x,y;public boolean onTouch(View v,MotionEvent e){FrameLayout.LayoutParams lp=(FrameLayout.LayoutParams)sheet.getLayoutParams();switch(e.getActionMasked()){
            case MotionEvent.ACTION_DOWN:downX=e.getRawX();downY=e.getRawY();x=lp.leftMargin;y=lp.topMargin;return true;
            case MotionEvent.ACTION_MOVE:host.apply(lp.width,lp.height,x+Math.round(e.getRawX()-downX),y+Math.round(e.getRawY()-downY));return true;
            case MotionEvent.ACTION_UP:host.save();return true;case MotionEvent.ACTION_CANCEL:return true;default:return true;}}});
        TextView close=action(a,"✕",0xff343546);header.addView(close);close.setOnClickListener(new View.OnClickListener(){public void onClick(View v){sheet.setVisibility(View.GONE);}});
        ScrollView scroll=new ScrollView(a);scroll.setFillViewport(false);scroll.setClipChildren(true);scroll.setClipToPadding(true);sheet.addView(scroll,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,0,1));
        LinearLayout body=new LinearLayout(a);body.setOrientation(LinearLayout.VERTICAL);body.setPadding(dp(a,16),dp(a,2),dp(a,16),dp(a,16));scroll.addView(body);
        TextView info=text(a,"Screen-space flashlight",12,MUTED,false);info.setPadding(0,0,0,dp(a,8));body.addView(info);
        toggle=action(a,light?"TURN BEAM OFF":"TURN BEAM ON",0xff6552bd);toggle.setEnabled(false);body.addView(toggle);toggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){light=!light;setEnabled(light);toggle.setText(light?"TURN BEAM OFF":"TURN BEAM ON");}});
        pauseToggle=action(a,"Pause-only controls   OFF",0xff343546);LinearLayout.LayoutParams opt=new LinearLayout.LayoutParams(-1,-2);opt.topMargin=dp(a,8);body.addView(pauseToggle,opt);
        pauseToggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){pauseOnly=!pauseOnly;prefs.edit().putBoolean("pauseOnly",pauseOnly).apply();queryScreen();updateVisibility();}});
        darkToggle=action(a,"Dark world   OFF",0xff343546);LinearLayout.LayoutParams darkParams=new LinearLayout.LayoutParams(-1,-2);darkParams.topMargin=dp(a,8);body.addView(darkToggle,darkParams);
        darkToggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){darkWorld=!darkWorld;prefs.edit().putBoolean("darkWorld",darkWorld).apply();updateDarkWorld();}});
        final TextView skyToggle=action(a,"Sky masking   "+(skyMask?"ON":"OFF"),0xff343546);LinearLayout.LayoutParams skyParams=new LinearLayout.LayoutParams(-1,-2);skyParams.topMargin=dp(a,8);body.addView(skyToggle,skyParams);
        skyToggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){skyMask=!skyMask;prefs.edit().putBoolean("skyMask",skyMask).apply();setSetting(10,skyMask?1f:0f);skyToggle.setText("Sky masking   "+(skyMask?"ON":"OFF"));}});
        skyShaftToggle=action(a,"Sky shaft only   "+(skyLook==1?"ON":"OFF"),0xff343546);LinearLayout.LayoutParams shaftParams=new LinearLayout.LayoutParams(-1,-2);shaftParams.topMargin=dp(a,8);body.addView(skyShaftToggle,shaftParams);
        skyShaftToggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){setSkyLook(UiPolicy.toggleSkyLook(skyLook,1));}});
        skyHaloToggle=action(a,"Sky soft halo   "+(skyLook==2?"ON":"OFF"),0xff343546);LinearLayout.LayoutParams haloParams=new LinearLayout.LayoutParams(-1,-2);haloParams.topMargin=dp(a,8);body.addView(skyHaloToggle,haloParams);
        skyHaloToggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){setSkyLook(UiPolicy.toggleSkyLook(skyLook,2));}});
        final TextView coneToggle=action(a,"Visible cone   "+(visibleCone?"ON":"OFF"),0xff343546);LinearLayout.LayoutParams coneParams=new LinearLayout.LayoutParams(-1,-2);coneParams.topMargin=dp(a,8);body.addView(coneToggle,coneParams);
        coneToggle.setOnClickListener(new View.OnClickListener(){public void onClick(View v){visibleCone=!visibleCone;prefs.edit().putBoolean("visibleCone",visibleCone).apply();setSetting(11,visibleCone?1f:0f);coneToggle.setText("Visible cone   "+(visibleCone?"ON":"OFF"));}});
        for(int i=0;i<2;i++){
            final int id=12+i;final String labelName=i==0?"Cone horizontal end":"Cone vertical end (top to bottom)";float def=i==0?.94f:.92f;
            float value=Math.max(0,Math.min(1,prefs.getFloat("setting"+id,def)));setSetting(id,value);
            LinearLayout row=new LinearLayout(a);row.setOrientation(LinearLayout.VERTICAL);row.setPadding(0,dp(a,7),0,0);body.addView(row);
            final TextView label=text(a,labelName+"   "+Math.round(value*100)+"%",12,TEXT,true);row.addView(label);
            SeekBar slider=new SeekBar(a);slider.setMax(100);slider.setProgress(Math.round(value*100));slider.setProgressTintList(ColorStateList.valueOf(ACCENT));slider.setThumbTintList(ColorStateList.valueOf(ACCENT));row.addView(slider);
            slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar s,int v,boolean fromUser){float x=v/100f;setSetting(id,x);label.setText(labelName+"   "+v+"%");if(fromUser&&prefs!=null)prefs.edit().putFloat("setting"+id,x).apply();}public void onStartTrackingTouch(SeekBar s){}public void onStopTrackingTouch(SeekBar s){}});
        }
        for(int i=0;i<NAMES.length;i++){
            final int id=i;float value=Math.max(0,Math.min(1,prefs.getFloat("setting"+i,DEFAULT[i])));if(i!=9)setSetting(i,value);
            LinearLayout row=new LinearLayout(a);row.setOrientation(LinearLayout.VERTICAL);row.setPadding(0,dp(a,7),0,0);body.addView(row);
            final TextView label=text(a,NAMES[i]+"   "+Math.round(value*100)+"%",12,TEXT,true);row.addView(label);
            SeekBar slider=new SeekBar(a);slider.setMax(100);slider.setProgress(Math.round(value*100));slider.setProgressTintList(ColorStateList.valueOf(ACCENT));slider.setThumbTintList(ColorStateList.valueOf(ACCENT));row.addView(slider);if(i==9)ambientSlider=slider;
            slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar s,int v,boolean fromUser){float x=v/100f;if(id!=9||darkWorld)setSetting(id,x);label.setText(NAMES[id]+"   "+v+"%");if(fromUser&&prefs!=null)prefs.edit().putFloat("setting"+id,x).apply();}public void onStartTrackingTouch(SeekBar s){}public void onStopTrackingTouch(SeekBar s){}});
        }
        updateDarkWorld();
        LinearLayout footer=new LinearLayout(a);footer.setGravity(Gravity.CENTER_VERTICAL);footer.setPadding(dp(a,14),dp(a,3),dp(a,8),dp(a,6));sheet.addView(footer);
        TextView hint=text(a,"Move from title",11,MUTED,false);footer.addView(hint,new LinearLayout.LayoutParams(0,-2,1));
        TextView resize=action(a,"↘  RESIZE",0xff343546);resize.setContentDescription("Drag to resize the Flashlight panel");footer.addView(resize);
        resize.setOnTouchListener(new View.OnTouchListener(){float downX,downY;int width,height,x,y;public boolean onTouch(View v,MotionEvent e){FrameLayout.LayoutParams lp=(FrameLayout.LayoutParams)sheet.getLayoutParams();switch(e.getActionMasked()){
            case MotionEvent.ACTION_DOWN:downX=e.getRawX();downY=e.getRawY();width=lp.width;height=lp.height;x=lp.leftMargin;y=lp.topMargin;return true;
            case MotionEvent.ACTION_MOVE:int maxW=Math.max(1,host.getWidth()-x-host.margin),maxH=Math.max(1,host.getHeight()-y-host.margin);
                int nw=Math.max(Math.min(host.minWidth,maxW),Math.min(maxW,width+Math.round(e.getRawX()-downX)));
                int nh=Math.max(Math.min(host.minHeight,maxH),Math.min(maxH,height+Math.round(e.getRawY()-downY)));host.apply(nw,nh,x,y);return true;
            case MotionEvent.ACTION_UP:host.save();return true;case MotionEvent.ACTION_CANCEL:return true;default:return true;}}});
        bubble=action(a,"FL",0xee6552bd);final TextView button=bubble;FrameLayout.LayoutParams bp=new FrameLayout.LayoutParams(dp(a,48),dp(a,48),Gravity.RIGHT|Gravity.CENTER_VERTICAL);bp.rightMargin=dp(a,12);root.addView(button,bp);
        button.setOnClickListener(new View.OnClickListener(){public void onClick(View v){sheet.setVisibility(sheet.getVisibility()==View.VISIBLE?View.GONE:View.VISIBLE);}});
        button.setOnTouchListener(new View.OnTouchListener(){float ox,oy,x,y;boolean moved;public boolean onTouch(View v,MotionEvent e){switch(e.getActionMasked()){
          case MotionEvent.ACTION_DOWN:ox=e.getRawX();oy=e.getRawY();x=v.getX();y=v.getY();moved=false;return true;
          case MotionEvent.ACTION_MOVE:float dx=e.getRawX()-ox,dy=e.getRawY()-oy;if(!moved&&dx*dx+dy*dy>dp(a,8)*dp(a,8))moved=true;if(moved){v.setX(Math.max(0,Math.min(root.getWidth()-v.getWidth(),x+dx)));v.setY(Math.max(0,Math.min(root.getHeight()-v.getHeight(),y+dy)));}return true;
          case MotionEvent.ACTION_UP:if(!moved)v.performClick();return true;case MotionEvent.ACTION_CANCEL:return true;default:return true;}}});
        a.addContentView(root,new ViewGroup.LayoutParams(-1,-1));root.post(new Runnable(){public void run(){if(root!=null)root.fit(root.getWidth(),root.getHeight());}});
    }
}
