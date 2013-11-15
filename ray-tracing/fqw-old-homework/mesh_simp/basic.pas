unit basic;

{$mode objfpc}{$H+}

interface

const
  MAX_MESH_COUNT = 2000000;
  MAX_POINT_COUNT = 2000000;

type
  KPoint = record
    x, y, z: Double;
  end;
  KMesh = array [0..2] of LongInt;
  KMatrix4 = array [0..3, 0..3] of Double;
  KVector4 = array [0..3] of Double;

procedure MatrixAdd(var Mx1: KMatrix4; const Mx2: KMatrix4);
function VectorMultMatrix(const Vec: KVector4; const Mat: KMatrix4): KVector4;
function VectorMultVector(const Vec1, Vec2: KVector4): Double;
function Det3(a1, b1, c1, a2, b2, c2, a3, b3, c3: Double): Double;
function GetCross(p1, p2: KPoint): KPoint;
function Minus(const p1, p2: KPoint): KPoint;
procedure Rotate2D(var x, y: Double; angle: Double);
procedure CutString(var s, s0: string);

implementation

procedure MatrixAdd(var Mx1: KMatrix4; const Mx2: KMatrix4);
var
  i, j: LongInt;
begin
  for i := 0 to 3 do
    for j := 0 to 3 do
      Mx1[i,j] := Mx1[i,j] + Mx2[i,j];
end;

function VectorMultMatrix(const Vec: KVector4; const Mat: KMatrix4): KVector4;
var
  i, j: LongInt;
  Ret: KVector4;
begin
  for i := 0 to 3 do
    Ret[i] := 0.0;
  for i  := 0 to 3 do
    for j := 0 to 3 do
      Ret[j] := Ret[j] + Vec[i] * Mat[i,j];
  Result := Ret;
end;

function VectorMultVector(const Vec1, Vec2: KVector4): Double;
var
  i: LongInt;
  Ret: Double;
begin
  Ret := 0.0;
  for i := 0 to 3 do
    Ret := Ret + Vec1[i] * Vec2[i];
  Result := Ret;
end;

function Det3(a1, b1, c1, a2, b2, c2, a3, b3, c3: Double): Double;
begin
  Result := a1*b2*c3 + b1*c2*a3 + c1*a2*b3 - a1*c2*b3 - b1*a2*c3 - c1*b2*a3;
end;

function GetCross(p1, p2: KPoint): KPoint;
begin
  Result.x := p1.y*p2.z - p1.z*p2.y;
  Result.y := p1.z*p2.x - p1.x*p2.z;
  Result.z := p1.x*p2.y - p1.y*p2.x;
end;

function Minus(const p1, p2: KPoint): KPoint;
begin
  Result.x := p2.x-p1.x;
  Result.y := p2.y-p1.y;
  Result.z := p2.z-p1.z;
end;

procedure Rotate2D(var x, y: Double; angle: Double);
var
  x0, y0: Double;
begin
  x0 := x * Cos(angle) - y * Sin(angle);
  y0 := x * Sin(angle) + y * Cos(angle);
  x := x0; y := y0;
end;

procedure CutString(var s, s0: string);
var
  p: LongInt;
begin
  while s[1] = ' ' do
    Delete(s, 1, 1);
  p := Pos(' ', s);
  if p <= 0 then
  begin
    s0 := s;
    s := '';
    Exit;
  end;
  s0 := Copy(s, 1, p-1);
  Delete(s, 1, p);
end;

end.

