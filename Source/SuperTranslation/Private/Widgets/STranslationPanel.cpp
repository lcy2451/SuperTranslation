// Fill out your copyright notice in the Description page of Project Settings.
#include "Widgets/STranslationPanel.h"

#include "HttpModule.h"
#include "SuperTranslation.h"
#include "Components/VerticalBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GenericPlatform/GenericPlatformHttp.h"
//#include "Settings/SuperTranslationSettings.h"
#include "Interfaces/IHttpResponse.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/SuperTranslationSettings.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"

#define MaximumLength 200
static const FName TestPluginATabName("TestPluginA");
FText STranslationPanel::FirstMenuLabel = FText::FromString(TEXT("检测语言"));
FText STranslationPanel::SecondMenuLabel = FText::FromString(TEXT("英语"));

FText STranslationPanel::FirstMenuLanguage = FText::FromString(TEXT("auto"));
FText STranslationPanel::SecondMenuLanguage = FText::FromString(TEXT("en"));

void STranslationPanel::GoogleTranslator(const FString& TextToTranslate, const FString& TargetLang,
	const FString& SourceLang)
{
	// 格式化 Google 极简接口的 URL
	FString ApiUrl = FString::Printf(
		TEXT("https://translate.googleapis.com/translate_a/single?client=gtx&sl=%s&tl=%s&dt=t&q=%s"),
		*SourceLang,
		*TargetLang,
		*FGenericPlatformHttp::UrlEncode(TextToTranslate) // 必须对中文进行 UrlEncode 编码
	);
	
	// 创建并发送 HTTP 请求
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiUrl);
	Request->SetVerb(TEXT("GET"));
	
	Request->OnProcessRequestComplete().BindRaw(this, &STranslationPanel::OnProcessRequestComplete);
	
	Request->ProcessRequest();
}

void STranslationPanel::BingTranslator(const FString& TextToTranslate, const FString& TargetLang,
	const FString& SourceLang)
{
	// 主要抄LanguageOne插件的代码
	// 使用 Microsoft Edge 浏览器内置翻译接口（无需 Key，免费且稳定）
	// 这是目前最推荐的免费方案，支持多语言，速度快
	
	// 第一步：获取 Authorization Token
	// 这个 Token 是动态的，必须每次（或定期）获取
	FString AuthUrl = TEXT("https://edge.microsoft.com/translate/auth");
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> AuthRequest = FHttpModule::Get().CreateRequest();
	AuthRequest->SetURL(AuthUrl);
	AuthRequest->SetVerb(TEXT("GET"));
	AuthRequest->SetHeader(TEXT("User-Agent"), TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0"));

	AuthRequest->OnProcessRequestComplete().BindRaw(this, &STranslationPanel::OnProcessBingRequestComplete);
	AuthRequest->ProcessRequest();
}

void STranslationPanel::OnProcessRequestComplete(FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
{
	if (bSuccess && Res.IsValid())
	{
		
		FString ResponseString = Res->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	        
		// 因为最外层是 [ 数组，而不是 { 对象
		TArray<TSharedPtr<FJsonValue>> OutermostArray;
		if (FJsonSerializer::Deserialize(Reader, OutermostArray) && OutermostArray.Num() > 0)
		{
			// 1. 获取第一层数组：[[["Hello", "你好", ...]]]
			TSharedPtr<FJsonValue> FirstLayer = OutermostArray[0];
			if (FirstLayer.IsValid() && FirstLayer->Type == EJson::Array)
			{
				const TArray<TSharedPtr<FJsonValue>>& SecondLayerArray = FirstLayer->AsArray();
				if (SecondLayerArray.Num() > 0)
				{
					// 2. 获取第二层数组：[["Hello", "你好", ...]]
					TSharedPtr<FJsonValue> SecondLayer = SecondLayerArray[0];
					if (SecondLayer.IsValid() && SecondLayer->Type == EJson::Array)
					{
						const TArray<TSharedPtr<FJsonValue>>& ThirdLayerArray = SecondLayer->AsArray();
						if (ThirdLayerArray.Num() > 0)
						{
							// 3. 获取第三层数组的第一个元素，即翻译后的字符串 "Hello"
							FString TranslatedText = ThirdLayerArray[0]->AsString();
	                            
							UE_LOG(LogTemp, Warning, TEXT("【JSON 正规解析成功】 翻译结果为: %s"), *TranslatedText);
							if (ConstructedSecondMultiLineEditableTextBox.IsValid())
							{
								ConstructedSecondMultiLineEditableTextBox->SetText(FText::FromString(TranslatedText));
								LastResult = TranslatedText;
							}
						}
					}
				}
			}
		}
	}
	
	if (ConstructedSThrobber.IsValid())
	{
		ConstructedSThrobber->SetVisibility(EVisibility::Hidden);
	}
}

void STranslationPanel::OnProcessBingRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
		if (!bSuccess || !Response.IsValid())
	{
		// OnError.ExecuteIfBound(TEXT("获取微软翻译授权失败，请检查网络 | Failed to get Microsoft auth"));
		UE_LOG(LogTemp, Warning, TEXT("获取微软翻译授权失败，请检查网络| Failed to get Microsoft auth"));
		return;
	}
	
	FString Token = Response->GetContentAsString();
	if (Token.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("获取到的微软授权Token为空 | Microsoft auth token is empty"));
		return;
	}
	
	// 第二步：使用 Token 调用翻译接口
	// Edge API 需要特定的语言代码格式 (例如中文必须是 zh-Hans)
	FString TargetLang = SecondMenuLanguage.ToString();
	FString EdgeTargetLang = TargetLang;
	if (EdgeTargetLang == TEXT("zh") || EdgeTargetLang == TEXT("zh-CN")) EdgeTargetLang = TEXT("zh-Hans");
	
	// API URL
	FString TranslateUrl = FString::Printf(TEXT("https://api-edge.cognitive.microsofttranslator.com/translate?from=&to=%s&api-version=3.0&includeSentenceLength=true"), *EdgeTargetLang);
		
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> TransRequest = FHttpModule::Get().CreateRequest();
	TransRequest->SetURL(TranslateUrl);
	TransRequest->SetVerb(TEXT("POST"));
	
	// 必须带上 Bearer Token
	TransRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Token));
	TransRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TransRequest->SetHeader(TEXT("User-Agent"), TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0"));
		
	// 构造请求体 [{"Text": "..."}]
	TSharedPtr<FJsonObject> TextObj = MakeShareable(new FJsonObject);
	FString SourceText = ConstructedFirstMultiLineEditableTextBox->GetText().ToString();
	TextObj->SetStringField(TEXT("Text"), SourceText);
	TArray<TSharedPtr<FJsonValue>> RequestArray;
	RequestArray.Add(MakeShareable(new FJsonValueObject(TextObj)));
	
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestArray, Writer);
	
	TransRequest->SetContentAsString(RequestBody);
	
	TransRequest->OnProcessRequestComplete().BindRaw(
		this, &STranslationPanel::OnProcessBingTransRequestComplete);
	
	TransRequest->ProcessRequest();
}

void STranslationPanel::OnProcessBingTransRequestComplete(FHttpRequestPtr TransReq, FHttpResponsePtr TransResp,
	bool bTransSuccess)
{
	if (!bTransSuccess || !TransResp.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("微软翻译请求失败 | Microsoft Translation request failed"));
		return;
	}
	
	FString TransRespStr = TransResp->GetContentAsString();
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(TransRespStr);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("解析微软翻译响应失败 | Failed to parse Microsoft response"));
		return;
	}
	
	// 响应格式: [{"translations":[{"text":"..."}]}]
	TSharedPtr<FJsonObject> FirstItem = JsonArray[0]->AsObject();
	if (FirstItem.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* Translations;
		if (FirstItem->TryGetArrayField(TEXT("translations"), Translations) && Translations->Num() > 0)
		{
			TSharedPtr<FJsonObject> FirstTrans = (*Translations)[0]->AsObject();
			if (FirstTrans.IsValid())
			{
				FString TranslatedText = FirstTrans->GetStringField(TEXT("text"));
				if (!TranslatedText.IsEmpty())
				{
					UE_LOG(LogTemp, Warning, TEXT("微软翻译成功 | %s"), *TranslatedText);
					ConstructedSecondMultiLineEditableTextBox->SetText(FText::FromString(TranslatedText));
					LastResult = TranslatedText;
				}
			}
		}
	}
	
	if (ConstructedSThrobber.IsValid())
	{
		ConstructedSThrobber->SetVisibility(EVisibility::Hidden);
	}
}

#define LOCTEXT_NAMESPACE "FSTranslationPanel"

void STranslationPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.MaxHeight(50.f)
		.MinHeight(50.f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				BuildFirstToolbar()
			]

			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FAppStyle::Get().GetBrush("Icons.arrow-leftright"))
				]
			]

			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[
				BuildSecondToolbar()
			]
		]

		+SVerticalBox::Slot()
		.MinHeight(500.f)
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			[
				ConstructFirstMultiLineEditableTextBox()
			]
					
			+SHorizontalBox::Slot()
			[
				ConstructSecondMultiLineEditableTextBox()
			]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			ConstructSThrobber()
		]
		
	];
}

TSharedRef<SWidget> STranslationPanel::BuildFirstToolbar()
{
	FSlimHorizontalToolBarBuilder Toolbar(nullptr, FMultiBoxCustomization::None);

	FUIAction ComboAction;
	Toolbar.AddComboButton(ComboAction, 
	FOnGetContent::CreateLambda(
		[&]()
		{
			FMenuBuilder Menu(true, nullptr);
			MakeFirstMenuEntries(Menu);

			return Menu.MakeWidget();
		}),
		TAttribute<FText>::Create(
			TAttribute<FText>::FGetter::CreateRaw(this, &STranslationPanel::GetFirstMenuLabel)),
		FText::GetEmpty(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.menu")
		
	);
	
	
	return Toolbar.MakeWidget();
}

TSharedRef<SWidget> STranslationPanel::BuildSecondToolbar()
{
	FSlimHorizontalToolBarBuilder Toolbar(nullptr, FMultiBoxCustomization::None);

	FUIAction ComboAction;
	Toolbar.AddComboButton(ComboAction, 
	FOnGetContent::CreateLambda(
		[&]()
		{
			FMenuBuilder Menu(true, nullptr);
			MakeSecondMenuEntries(Menu);

			return Menu.MakeWidget();
		}),
		TAttribute<FText>::Create(
			TAttribute<FText>::FGetter::CreateRaw(this, &STranslationPanel::GetSecondMenuLabel)),
		FText::GetEmpty(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.menu")
		
	);
	
	
	return Toolbar.MakeWidget();
}

void STranslationPanel::MakeFirstMenuEntries(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
	LOCTEXT("CheckLanguage", "检测语言"),
	FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.badge"),
	FUIAction(FExecuteAction::CreateRaw(
		this, &STranslationPanel::OnCheckFirstLanguageClicked, FString("DetectLanguage"))));
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Chinese", "中文(简体)"),
		FText::GetEmpty(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(),
			"Icons.badge"),
			FUIAction(FExecuteAction::CreateRaw(
			this, &STranslationPanel::OnCheckFirstLanguageClicked, FString("zh"))));
	
	MenuBuilder.AddMenuEntry(
	LOCTEXT("English", "英语"),
	FText::GetEmpty(),
	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.badge"),
	FUIAction(FExecuteAction::CreateRaw(
			this, &STranslationPanel::OnCheckFirstLanguageClicked, FString("en"))));
}

void STranslationPanel::MakeSecondMenuEntries(FMenuBuilder& MenuBuilder)
{
	
	MenuBuilder.AddMenuEntry(
	LOCTEXT("Chinese", "中文(简体)"),
	FText::GetEmpty(),
	FSlateIcon(FAppStyle::GetAppStyleSetName(),
		"Icons.badge"),
		FUIAction(FExecuteAction::CreateRaw(
		this, &STranslationPanel::OnCheckSecondLanguageClicked, FString("zh"))));
	
	MenuBuilder.AddMenuEntry(
	LOCTEXT("English", "英语"),
	FText::GetEmpty(),
	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.badge"),
	FUIAction(FExecuteAction::CreateRaw(
			this, &STranslationPanel::OnCheckSecondLanguageClicked, FString("en"))));
}

FText STranslationPanel::GetFirstMenuLabel()
{
	return FirstMenuLabel;
}

FText STranslationPanel::GetSecondMenuLabel()
{
	return SecondMenuLabel;
}

void STranslationPanel::OnCheckFirstLanguageClicked(const FString TargetLanguage)
{
	UE_LOG(LogTemp, Warning, TEXT("OnCheckFirstLanguageClicked"));
	FText* Label = LanguageToLanguageMap.Find(TargetLanguage);
	FirstMenuLabel = *Label;
	FirstMenuLanguage =  FText::FromString(TargetLanguage);
}

void STranslationPanel::OnCheckSecondLanguageClicked(const FString TargetLanguage)
{
	UE_LOG(LogTemp, Warning, TEXT("OnCheckSecondLanguageClicked"));
	FText* Label = LanguageToLanguageMap.Find(TargetLanguage);
	SecondMenuLabel = *Label;
	SecondMenuLanguage =  FText::FromString(TargetLanguage);
}

TSharedRef<SMultiLineEditableTextBox> STranslationPanel::ConstructFirstMultiLineEditableTextBox()
{
	ConstructedFirstMultiLineEditableTextBox = SNew(SMultiLineEditableTextBox)
			.OnTextCommitted_Raw(this, &STranslationPanel::OnTextCommitted)
			.OnTextChanged_Raw(this, &STranslationPanel::OnTextChanged);
	
	return ConstructedFirstMultiLineEditableTextBox.ToSharedRef();
}

TSharedRef<SMultiLineEditableTextBox> STranslationPanel::ConstructSecondMultiLineEditableTextBox()
{
	ConstructedSecondMultiLineEditableTextBox = SNew(SMultiLineEditableTextBox)
	.IsReadOnly(true)
	.AutoWrapText(false);
		
	return ConstructedSecondMultiLineEditableTextBox.ToSharedRef();
}

void STranslationPanel::OnTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (ConstructedSThrobber.IsValid())
	{
		ConstructedSThrobber->SetVisibility(EVisibility::Visible);
	}
	UE_LOG(LogTemp, Warning, TEXT("OnTextCommitted %s"), *NewText.ToString());
	
	// 待翻译的文本与目标语言
	FString TextToTranslate = NewText.ToString();
	FString TargetLang = SecondMenuLanguage.ToString();
	FString SourceLang = FirstMenuLanguage.ToString();
	
	if (TextToTranslate.IsEmpty() || LastInput == TextToTranslate)
	{
		if (ConstructedSThrobber.IsValid())
		{
			ConstructedSThrobber->SetVisibility(EVisibility::Hidden);
		}
		return;
	};
	
	LastInput = NewText.ToString();
	
	FSuperTranslationModule& SuperManagerModule = 
	FModuleManager::LoadModuleChecked<FSuperTranslationModule>(TEXT("SuperTranslation"));
	
	const USuperTranslationSettings* Settings = GetDefault<USuperTranslationSettings>();
	
	switch (Settings->TranslationEngine)
	{
	case ETranslationEngine::GoogleFree:
		UE_LOG(LogTemp, Warning, TEXT("On GoogleFree"));
		GoogleTranslator(TextToTranslate, TargetLang, SourceLang);
		break;
	case ETranslationEngine::MicrosoftFree:
		UE_LOG(LogTemp, Warning, TEXT("On MicrosoftFree"));
		BingTranslator(TextToTranslate, TargetLang, SourceLang);
		break;
	case ETranslationEngine::DeepSeekApi:
		UE_LOG(LogTemp, Warning, TEXT("On DeepSeekApi"));
		break;
	}
	
}

void STranslationPanel::OnTextChanged(const FText& NewText)
{
	const USuperTranslationSettings* Settings = GetDefault<USuperTranslationSettings>();
	
	// UE_LOG(LogTemp, Warning, TEXT("OnTextChanged %s"), Settings->TranslationEngine);
	// auto aa = Settings->TranslationEngine;
	//测试读取 插件设置
	switch (Settings->TranslationEngine)
	{
	case ETranslationEngine::GoogleFree:
		UE_LOG(LogTemp, Warning, TEXT("On GoogleFree"));
		break;
	case ETranslationEngine::MicrosoftFree:
		UE_LOG(LogTemp, Warning, TEXT("On MicrosoftFree"));
		break;
	case ETranslationEngine::DeepSeekApi:
		UE_LOG(LogTemp, Warning, TEXT("On DeepSeekApi"));
		break;
	}
	FString CurrentString = NewText.ToString();
	if (CurrentString.Len() > 20)
	{
		ConstructedFirstMultiLineEditableTextBox->SetText(FText::FromString(CurrentString.Left(20)));
	}
}

TSharedRef<SThrobber> STranslationPanel::ConstructSThrobber()
{
	TSharedRef<SThrobber> Throbber = SNew(SThrobber)
			.Animate(SThrobber::Opacity)
	.Visibility(EVisibility::Hidden);
	ConstructedSThrobber = Throbber.ToSharedPtr();
	return Throbber;
}

#undef LOCTEXT_NAMESPACE
