unit displayer;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Controls, OpenGLContext, GL, GLU, Math,
  Kernel, Basic;

const
  BACKGROUND: array [0..2] of GLfloat = (0.2, 0.1, 0.15);
  LIGHT_AMBIENT : array [0..3] of GLfloat = (0.2, 0.2, 0.2, 1.0);
  LIGHT_DIFFUSE : array [0..3] of GLfloat = (0.45, 0.5, 0.55, 1.0);
 // LIGHT_SPECULAR : array [0..3] of GLfloat = (1.0, 1.0, 1.0, 1.0);
  LIGHT_POSITION_0: array [0..3] of GLfloat = (1,1, 1, 1.0);
 // LIGHT_POSITION_1: array [0..3] of GLfloat = (10, 10, -10, 1.0);
 // OBJECT_AMBIENT  : array [0..3] of GLfloat = (1, 0, 0, 1);
//  OBJECT_DIFFUSE : array [0..3] of GLfloat = (0.7, 0.65, 0.55, 1.0);

type
  KDisplayer = class(TOpenGLControl)
  private
    MeshListID: GLuint;
    ViewpointX, ViewpointY, ViewPointZ: Double;
    MouseLastX, MouseLastY: Integer;
    MouseIsDown: Boolean;

    procedure PaintProcedure(Sender: TObject);
    procedure ResizeProcedure(Sender: TObject);
    procedure MouseDownProcedure(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure MouseMoveProcedure(Sender: TObject; Shift: TShiftState; X,
      Y: Integer);
    procedure MouseUpProcedure(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure MouseWheelProcedure(Sender: TObject; Shift: TShiftState;
      WheelDelta: Integer; MousePos: TPoint; var Handled: Boolean);
  public
    procedure Initalize;
    procedure InitGL;
    procedure ReloadModel(const Model: KModel; SmoothMode: Boolean);
  end;

implementation

procedure KDisplayer.Initalize;
begin
  ViewpointX := 0; ViewpointY := 0; ViewpointZ := 5;
  OnPaint := @PaintProcedure;
  OnMouseWheel := @MouseWheelProcedure;
  OnMouseDown := @MouseDownProcedure;
  OnMouseMove := @MouseMoveProcedure;
  OnMouseUp := @MouseUpProcedure;
  OnResize := @ResizeProcedure;
  InitGL;
 // glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
end;

procedure KDisplayer.InitGL;
begin
  MakeCurrent;

  glClearColor(BACKGROUND[0], BACKGROUND[1], BACKGROUND[2], 0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, double(Width) / Height, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glEnable(GL_LIGHTING);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, LIGHT_DIFFUSE);
  glLightfv(GL_LIGHT0, GL_AMBIENT, LIGHT_AMBIENT);
  glLightfv(GL_LIGHT0, GL_POSITION, LIGHT_POSITION_0);
  glEnable(GL_LIGHT0);

  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);
end;

procedure KDisplayer.MouseDownProcedure(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  MouseLastX := X; MouseLastY := Y;
  MouseIsDown := True;
end;

procedure KDisplayer.MouseMoveProcedure(Sender: TObject; Shift: TShiftState; X,
  Y: Integer);
var
  DeltaX, DeltaY, Limit, Epsilon, Tmp, NewTmp: Double;
begin
  if not MouseIsDown then
    Exit;
  DeltaX := (X - MouseLastX) / Width * Pi * 1.5;
  DeltaY := (Y - MouseLastY) / Height * Pi * 1.5;
  MouseLastX := X; MouseLastY := Y;

  Rotate2D(ViewpointX, ViewpointZ, DeltaX);
  Tmp := Sqrt(Sqr(ViewpointX) + Sqr(ViewpointZ));
  NewTmp := Tmp;
  Epsilon := 0.1;
  Limit := ArcTan2(ViewpointY, Tmp);
  if (Limit < 0) then
    Limit := Limit + Pi;
  if Limit < Pi/2 then
    DeltaY := Min(DeltaY, Pi/2 - Limit - Epsilon)
  else
    DeltaY := Max(DeltaY, Pi/2 - Limit + Epsilon);
  {
  if Limit < 0 then
    if Limit + DeltaY + Epsilon > 0 then
      DeltaY := -Limit - Epsilon
    else
  else
    if Limit + DeltaY - Epsilon < 0 then
      DeltaY := -Limit + Epsilon;  }
  Rotate2D(NewTmp, ViewpointY, DeltaY);
  ViewpointX := ViewpointX / Tmp * NewTmp;
  ViewpointZ := ViewpointZ / Tmp * NewTmp;
  Invalidate;
end;

procedure KDisplayer.MouseUpProcedure(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  MouseIsDown := False;
end;

procedure KDisplayer.MouseWheelProcedure(Sender: TObject; Shift: TShiftState;
  WheelDelta: Integer; MousePos: TPoint; var Handled: Boolean);
var
  Len, NewLen: Double;
begin
  Len := Sqrt(Sqr(ViewpointX) + Sqr(ViewpointY) + Sqr(ViewpointZ));
  if WheelDelta < 0 then
    NewLen := Len * (1+(-WheelDelta)/1000)
  else
    NewLen := Len / (1+WheelDelta/1000);
  ViewpointX := ViewpointX / Len * NewLen;
  ViewpointY := ViewpointY / Len * NewLen;
  ViewpointZ := ViewpointZ / Len * NewLen;
  Invalidate;
end;

procedure KDisplayer.PaintProcedure(Sender: TObject);
begin
  glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  gluLookAt(ViewpointX, ViewpointY, ViewpointZ, 0, 0, 0, 0, 1, 0);

  glPushMatrix;
    glColor3f(1, 1, 1);
    glCallList(MeshListID);
  glPopMatrix;

  SwapBuffers;
end;

procedure KDisplayer.ResizeProcedure(Sender: TObject);
begin
  InitGL;
  Invalidate;
end;

var
  TmpNormals: KNormalList;

procedure KDisplayer.ReloadModel(const Model: KModel; SmoothMode: Boolean);
var
  p0, p1, p2, n0: KPoint;
  i, j: LongInt;
begin
  glDeleteLists(MeshListID, 1);
  MeshListID := glGenLists(1);
  glNewList(MeshListID, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    with Model do
      if SmoothMode then
      begin
        ComputeNormals(Model, TmpNormals);
        for i := 0 to MeshCount-1 do
          for j := 0 to 2 do
          begin
            with TmpNormals[Meshes[i][j]] do
              glNormal3f(x, y, z);
            with Points[Meshes[i][j]] do
              glVertex3f(x, y, z);
          end;
      end else
      begin
        for i := 0 to MeshCount-1 do
        begin
          p0 := Points[Meshes[i][0]];
          p1 := Points[Meshes[i][1]];
          p2 := Points[Meshes[i][2]];
          n0 := GetCross(Minus(p1, p0), Minus(p2, p0));
          glNormal3f(n0.x, n0.y, n0.z);
          glVertex3f(p0.x, p0.y, p0.z);
          glVertex3f(p1.x, p1.y, p1.z);
          glVertex3f(p2.x, p2.y, p2.z);
        end;
      end;
    glEnd;
  glEndList;

  Invalidate;
end;

end.


