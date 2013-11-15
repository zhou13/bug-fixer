unit main;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, FileUtil, Forms, Controls, Graphics, Dialogs, ExtCtrls,
  Menus, StdCtrls, Spin, Buttons, ComCtrls, Math,
  Displayer, Kernel;

const
  MIN_MESHES_LIMIT = 10;
  TEXT_RUN_SIMPLIFY = '执行简化(&S)';
  TEXT_STOP_SIMPLIFY = '停止(&S)';

type

  { TMainform }

  TMainform = class(TForm)
    BtRecover: TButton;
    BtRun: TBitBtn;
    EdOriginal: TEdit;
    EdCurrent: TEdit;
    GroupBox1: TGroupBox;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    MainMenu: TMainMenu;
    MenuFile: TMenuItem;
    MenuItem1: TMenuItem;
    MenuItem2: TMenuItem;
    MenuExchangeXY: TMenuItem;
    MenuExchangeZY: TMenuItem;
    MenuItem3: TMenuItem;
    MenuDisplay: TMenuItem;
    MenuSmooth: TMenuItem;
    MenuRevNormal: TMenuItem;
    MenuRecover: TMenuItem;
    MenuItem4: TMenuItem;
    MenuRun: TMenuItem;
    MenuOperation: TMenuItem;
    MenuQuit: TMenuItem;
    MenuSave: TMenuItem;
    MenuOpen: TMenuItem;
    OpenDialog: TOpenDialog;
    Panel1: TPanel;
    DisplayPanel: TPanel;
    EdTarget: TSpinEdit;
    ProgressBar: TProgressBar;
    SaveDialog: TSaveDialog;
    StaticText1: TStaticText;
    StatusBar: TStatusBar;
    procedure BtRecoverClick(Sender: TObject);
    procedure BtRunClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var CloseAction: TCloseAction);
    procedure FormShow(Sender: TObject);
    procedure MenuExchangeXYClick(Sender: TObject);
    procedure MenuExchangeZYClick(Sender: TObject);
    procedure MenuOpenClick(Sender: TObject);
    procedure MenuQuitClick(Sender: TObject);
    procedure MenuRecoverClick(Sender: TObject);
    procedure MenuRevNormalClick(Sender: TObject);
    procedure MenuRunClick(Sender: TObject);
    procedure MenuSaveClick(Sender: TObject);
    procedure MenuSmoothClick(Sender: TObject);
  private
    Model, OriginalModel: KModel;
    Displayer: KDisplayer;
    RunningStartMeshes: LongInt;
    RunningTargetMeshes: LongInt;
    procedure Initalize;
    procedure OpenFile(FileName: String);
    procedure SaveFile(FileName: String);
    procedure ReloadModel;
    procedure RunSimplify;
    procedure ModelUpdateProcedure(Sender: TObject);
  public
    { public declarations }
  end;

var
  Mainform: TMainform;

implementation

{$R *.lfm}

{ TMainform }

procedure TMainform.RunSimplify;
var
  TargetMeshes: LongInt;
procedure UpdateControls(Running: Boolean);
begin
  MenuOpen.Enabled := not Running;
  MenuSave.Enabled := not Running;
  MenuQuit.Enabled := not Running;
  MenuRecover.Enabled := not Running;
  MenuExchangeXY.Enabled := not Running;
  MenuExchangeZY.Enabled := not Running;
  MenuRevNormal.Enabled := not Running;
  BtRecover.Enabled := not Running;
  EdTarget.ReadOnly := Running;
  ProgressBar.Visible := Running;
  if Running then
  begin
    BtRun.Caption := TEXT_STOP_SIMPLIFY;
    MenuRun.Caption := TEXT_STOP_SIMPLIFY;
  end else
  begin
    BtRun.Caption := TEXT_RUN_SIMPLIFY;
    MenuRun.Caption := TEXT_RUN_SIMPLIFY;
  end;
end;
begin
  TargetMeshes := EdTarget.Value;
  if (TargetMeshes < MIN_MESHES_LIMIT) then
  begin
    ShowMessage('目标面片过少，无法简化！');
    Exit;
  end;
  UpdateControls(True);
  Progressbar.Position := 0;

  RunningStartMeshes := Model.MeshCount;
  RunningTargetMeshes := TargetMeshes;
  KernelSimplifying := True;
  KernelStopSimplify := False;
  Sleep(100);
  Application.ProcessMessages;

  SimplifyModel(Model, TargetMeshes, @ModelUpdateProcedure);

  ReloadModel;
  KernelSimplifying := False;
  UpdateControls(False);
end;

procedure TMainform.ReloadModel;
begin
  Displayer.ReloadModel(Model, MenuSmooth.Checked);
  EdOriginal.Text := Format('%d', [OriginalModel.MeshCount]);
  EdCurrent.Text := Format('%d', [Model.MeshCount]);
  EdTarget.MaxValue := Model.MeshCount-1;
  EdTarget.MinValue := Min(MIN_MESHES_LIMIT, Model.MeshCount-1);
end;

procedure TMainform.OpenFile(FileName: String);
begin         //   showmessage('OpenFile begin');
  if not ReadModelFromFile(OriginalModel, FileName) then
    Exit;    //    showmessage('OpenFile Success!');
  CopyModel(OriginalModel, Model);
  Displayer.Initalize;
  ReloadModel;

  Statusbar.Panels[0].Text := '当前文件：' + FileName;
  MenuSave.Enabled := True;
  MenuOperation.Enabled := True;
  MenuDisplay.Enabled := True;
  BtRun.Enabled := True;
  BtRecover.Enabled := True;
  EdTarget.ReadOnly := False;
end;

procedure TMainform.SaveFile(FileName: String);
begin
  WriteModelToFile(Model, FileName);
end;

procedure TMainform.Initalize;
begin
  BtRun.Caption := TEXT_RUN_SIMPLIFY;
  MenuRun.Caption := TEXT_RUN_SIMPLIFY;
  MenuSave.Enabled := False;
  MenuOperation.Enabled := False;
  MenuDisplay.Enabled := False;
  BtRecover.Enabled := False;
  BtRun.Enabled := False;
  EdTarget.ReadOnly := True;
  ProgressBar.Visible := False;

  Displayer := KDisplayer.Create(DisplayPanel);
  Displayer.Parent := DisplayPanel;
  Displayer.Align := alClient;
  Displayer.Initalize;

  if FileExists(ParamStr(1)) then
  begin
 //   ShowMessage('FileExists: '+ParamStr(1));
    Application.ProcessMessages;
    OpenFile(ParamStr(1));
  end;
end;

procedure TMainform.ModelUpdateProcedure(Sender: TObject);
var
  FinishRate: Double;
begin
  FinishRate := (RunningStartMeshes-Model.MeshCount) / (RunningStartMeshes-RunningTargetMeshes);
  ProgressBar.Position := Round(ProgressBar.Max * FinishRate);
  ReloadModel;
  Application.ProcessMessages;
end;

procedure TMainform.FormShow(Sender: TObject);
begin
  Initalize;
end;

procedure TMainform.MenuExchangeXYClick(Sender: TObject);
var
  i: LongInt;
  Tmp: Double;
begin
  with Model do
    for i := 0 to PointCount-1 do
    begin
      Tmp := Points[i].x;
      Points[i].x := Points[i].y;
      Points[i].y := Tmp;
    end;
  with OriginalModel do
    for i := 0 to PointCount-1 do
    begin
      Tmp := Points[i].x;
      Points[i].x := Points[i].y;
      Points[i].y := Tmp;
    end;
  MenuRevNormalClick(Sender);
end;

procedure TMainform.MenuExchangeZYClick(Sender: TObject);
var
  i: LongInt;
  Tmp: Double;
begin
  with Model do
    for i := 0 to PointCount-1 do
    begin
      Tmp := Points[i].z;
      Points[i].z := Points[i].y;
      Points[i].y := Tmp;
    end;
  with OriginalModel do
    for i := 0 to PointCount-1 do
    begin
      Tmp := Points[i].z;
      Points[i].z := Points[i].y;
      Points[i].y := Tmp;
    end;
  MenuRevNormalClick(Sender);
end;

procedure TMainform.BtRunClick(Sender: TObject);
begin
  if KernelSimplifying then
    KernelStopSimplify := True
  else
    RunSimplify;
end;

procedure TMainform.FormClose(Sender: TObject; var CloseAction: TCloseAction);
begin
  if KernelSimplifying then
    KernelStopSimplify := True;
end;

procedure TMainform.BtRecoverClick(Sender: TObject);
begin
  CopyModel(OriginalModel, Model);
  ReloadModel;
end;

procedure TMainform.MenuOpenClick(Sender: TObject);
begin
  if OpenDialog.Execute then
    OpenFile(OpenDialog.FileName);
end;

procedure TMainform.MenuSaveClick(Sender: TObject);
var
  FileName: String;
begin
  if SaveDialog.Execute then
  begin
    FileName := SaveDialog.FileName;
    if (SaveDialog.FilterIndex = 1) and (ExtractFileExt(FileName) <> '.obj') then
      FileName := FileName + '.obj';
    SaveFile(FileName);
  end;
end;

procedure TMainform.MenuSmoothClick(Sender: TObject);
begin
  MenuSmooth.Checked := not MenuSmooth.Checked;
  ReloadModel;
end;

procedure TMainform.MenuQuitClick(Sender: TObject);
begin
  Close;
end;

procedure TMainform.MenuRecoverClick(Sender: TObject);
begin
  BtRecoverClick(Sender);
end;

procedure TMainform.MenuRevNormalClick(Sender: TObject);
var
  i: LongInt;
  Tmp: LongInt;
begin
  with Model do
    for i := 0 to MeshCount-1 do
    begin
      Tmp := Meshes[i][0];
      Meshes[i][0] := Meshes[i][1];
      Meshes[i][1] := Tmp;
    end;
  with OriginalModel do
    for i := 0 to MeshCount-1 do
    begin
      Tmp := Meshes[i][0];
      Meshes[i][0] := Meshes[i][1];
      Meshes[i][1] := Tmp;
    end;
  ReloadModel;
end;

procedure TMainform.MenuRunClick(Sender: TObject);
begin
  BtRunClick(Sender);
end;

end.

