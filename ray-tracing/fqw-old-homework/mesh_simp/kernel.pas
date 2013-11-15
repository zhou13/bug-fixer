unit kernel;

{$mode objfpc}{$H+}

interface

uses
  Classes, Dialogs, SysUtils, Math, Forms,
  Basic, DeleteSchemes;

type
  KModel = record
    Points: array [0..MAX_POINT_COUNT] of KPoint;
    Meshes: array [0..MAX_MESH_COUNT] of KMesh;
    PointCount, MeshCount: LongInt;
  end;

  KNormalList = array [0..MAX_POINT_COUNT] of KPoint;

var
  KernelSimplifying, KernelStopSimplify: Boolean;

procedure CopyModel(const Source: KModel; var Target: KModel);
function ReadModelFromFile(var Model: KModel; FileName: String): Boolean;
procedure WriteModelToFile(const Model: KModel; FileName: String);
procedure SimplifyModel(var Model: KModel; TargetMeshes: LongInt; UpdateProc: TNotifyEvent);
procedure ComputeNormals(const Model: KModel; var Normals: KNormalList);

implementation

var
  Link: array [0..MAX_POINT_COUNT] of LongInt;
  LinkMesh: array [0..MAX_MESH_COUNT*3] of LongInt;
  LinkNext: array [0..MAX_MESH_COUNT*3] of LongInt;
  PointMap: array [0..MAX_POINT_COUNT] of LongInt;
  MeshModified: array [0..MAX_MESH_COUNT] of Boolean;
  MeshK: array [0..MAX_MESH_COUNT] of KMatrix4;
  Schemes: array [0..MAX_MESH_COUNT*3] of KDeleteScheme;

procedure ComputeNormals(const Model: KModel; var Normals: KNormalList);
var
  p0, p1, p2, n0: KPoint;
  Len: Double;
  i, j: LongInt;
begin
  with Model do
  begin
    for i := 0 to PointCount-1 do
    begin
      Normals[i].x := 0.0;
      Normals[i].y := 0.0;
      Normals[i].z := 0.0;
    end;
    for i := 0 to MeshCount-1 do
    begin
      p0 := Points[Meshes[i][0]];
      p1 := Points[Meshes[i][1]];
      p2 := Points[Meshes[i][2]];
      n0 := GetCross(Minus(p1, p0), Minus(p2, p0));
      Len := Sqrt(Sqr(n0.x) + Sqr(n0.y) + Sqr(n0.z));
      n0.x := n0.x / Len;
      n0.y := n0.y / Len;
      n0.z := n0.z / Len;
      for j := 0 to 2 do
      begin
        Normals[Meshes[i][j]].x := Normals[Meshes[i][j]].x + n0.x;
        Normals[Meshes[i][j]].y := Normals[Meshes[i][j]].y + n0.y;
        Normals[Meshes[i][j]].z := Normals[Meshes[i][j]].z + n0.z;
      end;
    end;
  end;
end;

procedure SimplifyModel(var Model: KModel; TargetMeshes: LongInt; UpdateProc: TNotifyEvent);
var
  SchCnt: LongInt;
  function GetNextMeshesToDisplay(MeshCount: LongInt): LongInt;
  begin
    Result := MeshCount - 1000;
  end;
  procedure PreComputeK;
  var
    Pnt0, Pnt1, Pnt2, PntNormal: KPoint;
    Plane: KVector4;
    SSum: Double;
    i, j, k: LongInt;
  begin
    with Model do
      for i := 0 to MeshCount-1 do
        if MeshModified[i] then
        begin
          MeshModified[i] := False;
          Pnt0 := Points[Meshes[i][0]];
          Pnt1 := Points[Meshes[i][1]];
          Pnt2 := Points[Meshes[i][2]];
          PntNormal := GetCross(Minus(Pnt1, Pnt0), Minus(Pnt2, Pnt0));
          Plane[0] := PntNormal.x;
          Plane[1] := PntNormal.y;
          Plane[2] := PntNormal.z;
          SSum := Sqrt(Sqr(Plane[0]) + Sqr(Plane[1]) + Sqr(Plane[2]));
          Plane[0] := Plane[0] / SSum;
          Plane[1] := Plane[1] / SSum;
          Plane[2] := Plane[2] / SSum;
          Plane[3] := -(Plane[0]*Pnt0.x + Plane[1]*Pnt0.y + Plane[2]*Pnt0.z);
          for j := 0 to 3 do
            for k := 0 to 3 do
              MeshK[i][j,k] := Plane[j] * Plane[k];
        end;
  end;
  function GetQ(Pnt: LongInt): KMatrix4;
  var
    Mat: KMatrix4;
    i, j, Cur: LongInt;
  begin
    for i := 0 to 3 do
      for j := 0 to 3 do
        Mat[i,j] := 0.0;
    Cur := Link[Pnt];
    while Cur >= 0 do
    begin
      MatrixAdd(Mat, MeshK[LinkMesh[Cur]]);
      Cur := LinkNext[Cur];
    end;
    Result := Mat;
  end;
  function GetDeleteScheme(Pnt1, Pnt2: LongInt): KDeleteScheme;
  var
    Ret: KDeleteScheme;
    Q: KMatrix4;
    D, Dx, Dy, Dz: Double;
    Vec1, Vec2: KVector4;
  begin
    Q := GetQ(Pnt1);
    MatrixAdd(Q, GetQ(Pnt2));
    D := Det3(
         Q[0,0], Q[0,1], Q[0,2],
         Q[1,0], Q[1,1], Q[1,2],
         Q[2,0], Q[2,1], Q[2,2]);
    if (Abs(D) < 1E-12) then
    begin
      Ret.Target.x := (Model.Points[Pnt1].x + Model.Points[Pnt2].x) * 0.5;
      Ret.Target.y := (Model.Points[Pnt1].y + Model.Points[Pnt2].y) * 0.5;
      Ret.Target.z := (Model.Points[Pnt1].z + Model.Points[Pnt2].z) * 0.5;
    end else
    begin
      Dx := Det3(
         -Q[0,3], Q[0,1], Q[0,2],
         -Q[1,3], Q[1,1], Q[1,2],
         -Q[2,3], Q[2,1], Q[2,2]);
      Dy := Det3(
         Q[0,0], -Q[0,3], Q[0,2],
         Q[1,0], -Q[1,3], Q[1,2],
         Q[2,0], -Q[2,3], Q[2,2]);
      Dz := Det3(
         Q[0,0], Q[0,1], -Q[0,3],
         Q[1,0], Q[1,1], -Q[1,3],
         Q[2,0], Q[2,1], -Q[2,3]);
      Ret.Target.x := Dx / D;
      Ret.Target.y := Dy / D;
      Ret.Target.z := Dz / D;
    end;
    Ret.Pnt1 := Pnt1;
    Ret.Pnt2 := Pnt2;

    Vec1[0] := Ret.Target.x;
    Vec1[1] := Ret.Target.y;
    Vec1[2] := Ret.Target.z;
    Vec1[3] := 1.0;
    Vec2 := VectorMultMatrix(Vec1, Q);
    Ret.Cost := VectorMultVector(Vec1, Vec2);

    Result := Ret;
  end;
  function GetCostLimit: Double;
  var
    Limit: Double;
    i: LongInt;
  begin
    Limit := Max(0.0, Schemes[0].Cost * 1.5);
    i := Min(SchCnt div 4 - 1000, 50000);
    if i >= 0 then
      Limit := Max(Limit, Schemes[i].Cost);
    Result := Limit;
  end;
  function ApplyDeleteScheme(const Sch: KDeleteScheme): LongInt;
  var
    i, Cur: LongInt;
    Pnt: array [0..1] of LongInt;
  begin
    Pnt[0] := Sch.Pnt1;
    Pnt[1] := Sch.Pnt2;
    for i := 0 to 1 do
    begin
      Cur := Link[Pnt[i]];
      while Cur >= 0 do
      begin
        if MeshModified[LinkMesh[Cur]] then
        begin
          Result := 0;
          Exit;
        end;
        Cur := LinkNext[Cur];
      end;
    end;
    for i := 0 to 1 do
    begin
      Cur := Link[Pnt[i]];
      while Cur >= 0 do
      begin
        MeshModified[LinkMesh[Cur]] := True;
        Cur := LinkNext[Cur];
      end;
    end;
    PointMap[Sch.Pnt2] := Sch.Pnt1;
    Model.Points[Sch.Pnt1] := Sch.Target;
    Result := 2;
  end;
var
  NextMeshesToDisplay: LongInt;
  i, j, Rest: LongInt;
  CostLimit: Double;
begin
  with Model do
  begin
    NextMeshesToDisplay := GetNextMeshesToDisplay(MeshCount);
    for i := 0 to MeshCount-1 do
      MeshModified[i] := True;
    while (not KernelStopSimplify) and (Model.MeshCount > TargetMeshes) do
    begin
      //  showmessage('start PreComputeK');
      PreComputeK();
      //  showmessage('end PreComputeK');
      SchCnt := 0;
      for i := 0 to PointCount-1 do
        PointMap[i] := i;
      for i := 0 to PointCount-1 do
        Link[i] := -1;
      for i := 0 to MeshCount-1 do
        for j := 0 to 2 do
        begin
          LinkMesh[SchCnt] := i;
          LinkNext[SchCnt] := Link[Meshes[i][j]];
          Link[Meshes[i][j]] := SchCnt;
          Inc(SchCnt);
        end;
      SchCnt := 0;
      for i := 0 to MeshCount-1 do
      begin
        for j := 0 to 2 do
        begin
          Application.ProcessMessages;
          if KernelStopSimplify then
            Break;
          Schemes[SchCnt] := GetDeleteScheme(Meshes[i][j], Meshes[i][(j+1) mod 3]);
          Inc(SchCnt);
        end;
        if KernelStopSimplify then
          Break;
      end;
      if KernelStopSimplify then
        Break;
      SortDeleteSchemes(Schemes, 0, SchCnt-1);
      CostLimit := GetCostLimit();
      Rest := MeshCount - TargetMeshes;
      for i := 0 to SchCnt-1 do
      begin
        if Schemes[i].Cost > CostLimit then
          Break;
        Application.ProcessMessages;
        if KernelStopSimplify then
          Break;
        Rest := Rest - ApplyDeleteScheme(Schemes[i]);
        if Rest <= 0 then
          Break;
      end;
      i := 0;
      while i < MeshCount do
        if MeshModified[i] then
        begin
          for j := 0 to 2 do
            Meshes[i][j] := PointMap[Meshes[i][j]];
          if (Meshes[i][0] = Meshes[i][1]) or (Meshes[i][0] = Meshes[i][2]) or (Meshes[i][1] = Meshes[i][2]) then
          begin
            Meshes[i] := Meshes[MeshCount-1];
            MeshModified[i] := MeshModified[MeshCount-1];
            MeshK[i] := MeshK[MeshCount-1];
            Dec(MeshCount);
          end else
            Inc(i);
        end else
          Inc(i);
      if MeshCount < NextMeshesToDisplay then
      begin
        UpdateProc(Nil);
        NextMeshesToDisplay := GetNextMeshesToDisplay(MeshCount);
      end;
    end;
    for i := 0 to PointCount-1 do
      PointMap[i] := 0;
    for i := 0 to MeshCount-1 do
      for j := 0 to 2 do
        PointMap[Meshes[i][j]] := 1;
    j := PointCount;
    PointCount := 0;
    for i := 0 to j-1 do
      if PointMap[i] > 0 then
      begin
        PointMap[i] := PointCount;
        Points[PointMap[i]] := Points[i];
        Inc(PointCount);
      end;
    for i := 0 to MeshCount-1 do
      for j := 0 to 2 do
        Meshes[i][j] := PointMap[Meshes[i][j]];
  end;
end;

procedure CopyModel(const Source: KModel; var Target: KModel);
var
  i: LongInt;
begin
  Target.PointCount := Source.PointCount;
  Target.MeshCount := Source.MeshCount;
  for i := 0 to Target.PointCount-1 do
    Target.Points[i] := Source.Points[i];
  for i := 0 to Target.MeshCount-1 do
    Target.Meshes[i] := Source.Meshes[i];
end;

function ReadModelFromFile(var Model: KModel; FileName: String): Boolean;
var
  List: TStringList;
  i, j, TmpPoints, TmpMeshes: LongInt;
  s, s2: String;
begin                 // showmessage('ReadModelFromFile');
  List := TStringList.Create;
  List.LoadFromFile(FileName);
           //   showmessage(format('list.size=%d',[list.count]));
  s := List[0];
  CutString(s, s2);
  CutString(s, s2); Val(s2, TmpPoints);
  CutString(s, s2); Val(s2, TmpMeshes);
  if (TmpPoints > MAX_POINT_COUNT) or (TmpMeshes > MAX_MESH_COUNT) then
  begin
    ShowMessage('读取失败：面片数或点数过多！');
    Exit(False);
  end;
  Model.PointCount := TmpPoints;
  Model.MeshCount := TmpMeshes;
  for i := 0 to Model.PointCount-1 do
  begin
    s := List[1 + i];
    CutString(s, s2);
    CutString(s, s2); Val(s2, Model.Points[i].x);
    CutString(s, s2); Val(s2, Model.Points[i].y);
    CutString(s, s2); Val(s2, Model.Points[i].z);
  end;
  for i := 0 to Model.MeshCount-1 do
  begin
    s := List[1 + Model.PointCount + i];
    CutString(s, s2);
    for j := 0 to 2 do
    begin
      CutString(s, s2); Val(s2, Model.Meshes[i][j]);
      Dec(Model.Meshes[i][j]);
    end;
  end;
  List.Free;
  Exit(True);
end;

procedure WriteModelToFile(const Model: KModel; FileName: String);
var
  List: TStringList;
  i: LongInt;
begin
  List := TStringList.Create;
  with Model do
  begin
    List.Add(Format('# %d %d', [PointCount, MeshCount]));
    for i := 0 to Model.PointCount-1 do
      List.Add(Format('v %.15f %.15f %.15f', [Points[i].x, Points[i].y, Points[i].z]));
    for i := 0 to Model.MeshCount-1 do
      List.Add(Format('f %d %d %d', [Meshes[i][0]+1, Meshes[i][1]+1, Meshes[i][2]+1]));
  end;
  List.SaveToFile(FileName);
  List.Free;
end;

end.

