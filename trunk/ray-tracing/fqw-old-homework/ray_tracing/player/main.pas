unit main;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, FileUtil, Forms, Controls, Graphics, Dialogs, StdCtrls,
  ComCtrls, ExtCtrls, Spin, Buttons, Windows;

type

  { TMainForm }

  TMainForm = class(TForm)
    BitBtn1: TBitBtn;
    edSizeX: TSpinEdit;
    edPosiX: TSpinEdit;
    edFlush: TSpinEdit;
    edSizeY: TSpinEdit;
    edPosiY: TSpinEdit;
    GroupBox2: TGroupBox;
    GroupBox3: TGroupBox;
    GroupBox4: TGroupBox;
    Image1: TImage;
    Image2: TImage;
    Image3: TImage;
    Image4: TImage;
    Image5: TImage;
    Image6: TImage;
    Image7: TImage;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    Label7: TLabel;
    Pages: TPageControl;
    Panel1: TPanel;
    Panel2: TPanel;
    Panel3: TPanel;
    StaticText1: TStaticText;
    StaticText10: TStaticText;
    StaticText11: TStaticText;
    StaticText12: TStaticText;
    StaticText13: TStaticText;
    StaticText3: TStaticText;
    StaticText4: TStaticText;
    StaticText5: TStaticText;
    StaticText6: TStaticText;
    StaticText7: TStaticText;
    StaticText8: TStaticText;
    StaticText9: TStaticText;
    TabSheet1: TTabSheet;
    TabSheet2: TTabSheet;
    TabSheet3: TTabSheet;
    TabSheet4: TTabSheet;
    TabSheet5: TTabSheet;
    TabSheet6: TTabSheet;
    TabSheet7: TTabSheet;
    procedure BitBtn1Click(Sender: TObject);
  private
    { private declarations }
  public
    { public declarations }
  end; 

var
  MainForm: TMainForm;

implementation


{$R *.lfm}

{ TMainForm }

{ TMainForm }

procedure TMainForm.BitBtn1Click(Sender: TObject);
var
  filename, parameters: string;
begin
  filename := Format('scene%d\configs.ini', [Pages.PageCount-Pages.ActivePageIndex]);
  parameters := Format('%s %d %d %d %d %d', [filename, edSizeX.Value, edSizeY.Value, edPosiX.Value, edPosiY.Value, edFlush.Value]);
  ShellExecute(0, 'open', 'ray_tracing_main.exe', PChar(parameters), PChar(ExtractFilePath(ParamStr(0))+'scenes'), SW_SHOWNORMAL);
//  WinExec(PChar(cmd), SW_SHOW);
end;

end.

