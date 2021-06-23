package svtk;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;

import javax.swing.Timer;

/**
 * Java AWT component that encapsulate svtkGenericRenderWindowInteractor,
 * svtkPlaneWidget, svtkBoxWidget and extends svtkPanel
 *
 * @see svtkPanel
 * @author Kitware
 */
public class svtkCanvas extends svtkPanel implements MouseListener, MouseMotionListener, MouseWheelListener, KeyListener {
  private static final long serialVersionUID = 1L;
  protected svtkGenericRenderWindowInteractor iren = new svtkGenericRenderWindowInteractor();
  protected Timer timer = new Timer(10, new DelayAction());
  protected int ctrlPressed = 0;
  protected int shiftPressed = 0;
  protected svtkPlaneWidget pw = new svtkPlaneWidget();
  protected svtkBoxWidget bw = new svtkBoxWidget();

  public void Delete() {
    iren = null;
    pw = null;
    bw = null;
    super.Delete();
  }

  public svtkCanvas() {
    super();
    Initialize();
  }

  public svtkCanvas(svtkRenderWindow renwin) {
    super(renwin);
    Initialize();
  }

  protected void Initialize() {
    iren.SetRenderWindow(rw);
    iren.TimerEventResetsTimerOff();
    iren.AddObserver("CreateTimerEvent", this, "StartTimer");
    iren.AddObserver("DestroyTimerEvent", this, "DestroyTimer");
    iren.SetSize(200, 200);
    iren.ConfigureEvent();
    pw.AddObserver("EnableEvent", this, "BeginPlaneInteraction");
    bw.AddObserver("EnableEvent", this, "BeginBoxInteraction");
    pw.SetKeyPressActivationValue('l');
    bw.SetKeyPressActivationValue('b');
    pw.SetInteractor(iren);
    bw.SetInteractor(iren);

    addComponentListener(new ComponentAdapter() {
      public void componentResized(ComponentEvent event) {
        // The Canvas is being resized, get the new size
        int width = getWidth();
        int height = getHeight();
        setSize(width, height);
      }
    });

    ren.SetBackground(0.0, 0.0, 0.0);

    // Setup same interactor style than svtkPanel
    svtkInteractorStyleTrackballCamera style = new svtkInteractorStyleTrackballCamera();
    iren.SetInteractorStyle(style);
  }

  public void StartTimer() {
    if (timer.isRunning())
      timer.stop();

    timer.setRepeats(true);
    timer.start();
  }

  public void DestroyTimer() {
    if (timer.isRunning())
      timer.stop();
  }

  /**
   * Replace by getRenderWindowInteractor()
   */
  @Deprecated
  public svtkGenericRenderWindowInteractor getIren() {
    return this.iren;
  }

  public svtkGenericRenderWindowInteractor getRenderWindowInteractor() {
    return this.iren;
  }

  public void setInteractorStyle(svtkInteractorStyle style) {
    iren.SetInteractorStyle(style);
  }

  public void addToPlaneWidget(svtkProp3D prop) {
    pw.SetProp3D(prop);
    pw.PlaceWidget();
  }

  public void addToBoxWidget(svtkProp3D prop) {
    bw.SetProp3D(prop);
    bw.PlaceWidget();
  }

  public void BeginPlaneInteraction() {
    System.out.println("Plane widget begin interaction");
  }

  public void BeginBoxInteraction() {
    System.out.println("Box widget begin interaction");
  }

  public void setSize(int x, int y) {
    super.setSize(x, y);
    if (windowset == 1) {
      Lock();
      rw.SetSize(x, y);
      iren.SetSize(x, y);
      iren.ConfigureEvent();
      UnLock();
    }
  }

  public void mouseClicked(MouseEvent e) {
  }

  public void mousePressed(MouseEvent e) {
    if (ren.VisibleActorCount() == 0)
      return;
    Lock();
    rw.SetDesiredUpdateRate(5.0);
    lastX = e.getX();
    lastY = e.getY();

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;

    iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed, shiftPressed, '0', 0, "0");

    if ((e.getModifiers() & InputEvent.BUTTON1_MASK) == InputEvent.BUTTON1_MASK) {
      iren.LeftButtonPressEvent();
    } else if ((e.getModifiers() & InputEvent.BUTTON2_MASK) == InputEvent.BUTTON2_MASK) {
      iren.MiddleButtonPressEvent();
    } else if ((e.getModifiers() & InputEvent.BUTTON3_MASK) == InputEvent.BUTTON3_MASK) {
      iren.RightButtonPressEvent();
    }
    UnLock();
  }

  public void mouseReleased(MouseEvent e) {
    rw.SetDesiredUpdateRate(0.01);

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;

    iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed, shiftPressed, '0', 0, "0");

    if ((e.getModifiers() & InputEvent.BUTTON1_MASK) == InputEvent.BUTTON1_MASK) {
      Lock();
      iren.LeftButtonReleaseEvent();
      UnLock();
    }

    if ((e.getModifiers() & InputEvent.BUTTON2_MASK) == InputEvent.BUTTON2_MASK) {
      Lock();
      iren.MiddleButtonReleaseEvent();
      UnLock();
    }

    if ((e.getModifiers() & InputEvent.BUTTON3_MASK) == InputEvent.BUTTON3_MASK) {
      Lock();
      iren.RightButtonReleaseEvent();
      UnLock();
    }
  }

  public void mouseEntered(MouseEvent e) {
    this.requestFocus();
    iren.SetEventInformationFlipY(e.getX(), e.getY(), 0, 0, '0', 0, "0");
    iren.EnterEvent();
  }

  public void mouseExited(MouseEvent e) {
    iren.SetEventInformationFlipY(e.getX(), e.getY(), 0, 0, '0', 0, "0");
    iren.LeaveEvent();
  }

  public void mouseMoved(MouseEvent e) {
    lastX = e.getX();
    lastY = e.getY();

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;

    iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed, shiftPressed, '0', 0, "0");

    Lock();
    iren.MouseMoveEvent();
    UnLock();
  }

  public void mouseDragged(MouseEvent e) {
    if (ren.VisibleActorCount() == 0)
      return;

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;

    iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed, shiftPressed, '0', 0, "0");

    Lock();
    iren.MouseMoveEvent();
    UnLock();

    UpdateLight();
  }

  public void mouseWheelMoved(MouseWheelEvent e) {
    if (ren.VisibleActorCount() == 0)
      return;

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;

    Lock();
    if (e.getWheelRotation() > 0) {
      iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed, shiftPressed, '0', 0, "0");
      iren.MouseWheelBackwardEvent();
    }
    else if (e.getWheelRotation() < 0) {
      iren.SetEventInformationFlipY(e.getX(), e.getY(), ctrlPressed, shiftPressed, '0', 0, "0");
      iren.MouseWheelForwardEvent();
    }
    UnLock();

    UpdateLight();
  }

  public void keyTyped(KeyEvent e) {
  }

  public void keyPressed(KeyEvent e) {
    if (ren.VisibleActorCount() == 0)
      return;
    char keyChar = e.getKeyChar();

    ctrlPressed = (e.getModifiers() & InputEvent.CTRL_MASK) == InputEvent.CTRL_MASK ? 1 : 0;
    shiftPressed = (e.getModifiers() & InputEvent.SHIFT_MASK) == InputEvent.SHIFT_MASK ? 1 : 0;

    iren.SetEventInformationFlipY(lastX, lastY, ctrlPressed, shiftPressed, keyChar, 0, String.valueOf(keyChar));

    Lock();
    iren.KeyPressEvent();
    iren.CharEvent();
    UnLock();
  }

  public void keyReleased(KeyEvent e) {
  }

  private class DelayAction implements ActionListener {
    public void actionPerformed(ActionEvent evt) {
      Lock();
      iren.TimerEvent();
      UpdateLight();
      UnLock();
    }
  }
}
