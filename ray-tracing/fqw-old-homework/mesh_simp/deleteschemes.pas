unit deleteschemes;

{$mode objfpc}{$H+}

interface

uses
  Basic;

type
  KDeleteScheme = record
    Pnt1, Pnt2: LongInt;
    Target: KPoint;
    Cost: Double;
  end;

procedure SortDeleteSchemes(var Schemes: array of KDeleteScheme; First, Last: LongInt);

implementation

procedure SortDeleteSchemes(var Schemes: array of KDeleteScheme; First, Last: LongInt);
var
  i, j: LongInt;
  MidCost: Double;
  Tmp: KDeleteScheme;
begin
  if First >= Last then
    Exit;
  i := First; j := Last;
  MidCost := Schemes[(i+j) shr 1].Cost;
  repeat
    while Schemes[i].Cost < MidCost do
      Inc(i);
    While MidCost < Schemes[j].Cost do
      Dec(j);
    if i <= j then
    begin
      Tmp := Schemes[i];
      Schemes[i] := Schemes[j];
      Schemes[j] := Tmp;
      Inc(i); Dec(j);
    end;
  until i > j;
  SortDeleteSchemes(Schemes, First, j);
  SortDeleteSchemes(Schemes, i, Last);
end;

end.

