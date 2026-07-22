// Fill out your copyright notice in the Description page of Project Settings.


#include "Providers/DeepSeekProvider.h"

#include "HttpModule.h"
#include "Dom/JsonObject.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/SuperTranslationSettings.h"
#include "Widgets/STranslationPanel.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

FString EscapeJson(const FString& Text)
{
	FString Result = Text;

	Result.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	Result.ReplaceInline(TEXT("\""), TEXT("\\\""));
	Result.ReplaceInline(TEXT("\r\n"), TEXT("\\n"));
	Result.ReplaceInline(TEXT("\n"), TEXT("\\n"));
	Result.ReplaceInline(TEXT("\t"), TEXT("\\t"));

	return Result;
}

DeepSeekProvider::DeepSeekProvider(TWeakPtr<STranslationPanel> TranslationPanelWeakPtr)
{
	TranslationPanel = TranslationPanelWeakPtr;
}

DeepSeekProvider::DeepSeekProvider()
{
}

DeepSeekProvider::~DeepSeekProvider()
{
}

void DeepSeekProvider::Translator(FString Text, FString TargetLang, FString SourceLang, const FString& Model)
{
	if (TargetLang == TEXT("en"))
	{
		TargetLang = "English";
	}
	else if (TargetLang == TEXT("zh"))
	{
		TargetLang = "Simplified Chinese";
	}
	

	if (SourceLang == TEXT("en"))
	{
		SourceLang = "English";
	}
	else if (SourceLang == TEXT("zh"))
	{
		SourceLang = "Simplified Chinese";
	}

	FString TranslateInstruction;
	if (SourceLang == TEXT("auto"))
	{
		TranslateInstruction = FString::Printf(
		TEXT("Automatically detect the source language and translate the Unreal Engine related text into %s."),
		*TargetLang
		);
	}
	else
	{
		TranslateInstruction = FString::Printf(
			TEXT("Translate Unreal Engine related %s text into %s."),
			*SourceLang,
			*TargetLang
		);
	}
	
	/**
	 * 构建用于 DeepSeek 翻译请求的系统提示词。
	 *
	 * 规则说明：
	 *
	 * 1. 优先使用 Unreal Engine 官方目标语言术语。
	 *
	 * 2. 在合适的情况下保留 Unreal 类名和 API 名称，不进行翻译。
	 *
	 * 3. 对含义不明确的缩写，不进行扩展、解释或猜测，保持原样。
	 *
	 * 4. 除非源文本中的内容有明确翻译，否则保留每一个源文本 token。
	 *
	 * 5. 不省略、删除、纠正、规范化或重写未知单词、标识符、
	 *    缩写、符号或格式异常的文本。
	 *
	 * 6. 对未知或无法翻译的内容，必须在输出中保持原样。
	 *
	 * 7. 只返回符合指定结构的合法 JSON：
	 *
	 *    {
	 *        "translation": "string",
	 *        "alternatives": [
	 *            "string"
	 *        ]
	 *    }
	 *
	 * 8. JSON 字段名必须严格为 "translation" 和 "alternatives"。
	 *
	 * 9. 不允许重命名、增加或删除任何 JSON 字段。
	 *
	 * 10. "translation" 字段必须是字符串。
	 *
	 * 11. "alternatives" 字段必须是字符串数组。
	 *
	 * 12. 不返回 Markdown，也不在 JSON 对象之外添加任何文字。
	 *
	 * 13. 示例中要求输入 "sdas hello" 时必须保留 "sdas"，
	 *     例如返回：
	 *
	 *     {
	 *         "translation": "sdas 你好",
	 *         "alternatives": [
	 *             "sdas 您好"
	 *         ]
	 *     }
	 *
	 * 格式化参数：
	 * - 第一个 %s：TranslateInstruction，翻译方向说明。
	 * - 第二个 %s：TargetLang，目标语言名称。
	 */

	FString SystemPrompt = FString::Printf(
	TEXT(
		"You are a professional Unreal Engine technical translator.\n\n"
		"%s\n\n"
		"Rules:\n"
		"1. Use official Unreal Engine %s terminology when available.\n"
		"2. Keep Unreal class names and API names unchanged when appropriate.\n"
		"3. Do not expand, interpret, or guess ambiguous abbreviations. Keep them unchanged.\n"
		"4. Preserve every source token unless it has a clear translation.\n"
		"5. Do not omit, delete, correct, normalize, or rewrite unknown words, identifiers, abbreviations, symbols, or malformed text.\n"
		"6. Unknown or untranslatable content must remain unchanged in the output.\n"
		"7. Return only valid JSON using exactly this structure:\n"
		"{\"translation\":\"string\",\"alternatives\":[\"string\"]}\n"
		"8. The key names must be exactly \"translation\" and \"alternatives\".\n"
		"9. Do not rename, add, or remove any keys.\n"
		"10. \"translation\" must be a string.\n"
		"11. \"alternatives\" must be an array of strings.\n"
		"12. Return no Markdown and no text outside the JSON object.\n"
		"13. Example: Input \"sdas hello\" must preserve \"sdas\", such as {\"translation\":\"sdas 你好\",\"alternatives\":[\"sdas 您好\"]}."
	),
	*TranslateInstruction,
	*TargetLang
	);
	
	const USuperTranslationSettings* Settings = GetDefault<USuperTranslationSettings>();
	
	FString AuthUrl = TEXT("https://api.deepseek.com/chat/completions");
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(AuthUrl);
	// 这里必须是 POST
	Request->SetVerb(TEXT("POST"));
	
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetHeader(
		TEXT("Authorization"),
		FString::Printf(TEXT("Bearer %s"), *Settings->DeepSeekApiKey)
	);
	
	FString JsonBody =  FString::Printf(TEXT(R"(
		{
		  "messages": [
		    {
		      "content": "%s",
		      "role": "system"
		    },
		    {
		      "content": "%s",
		      "role": "user"
		    }
		  ],
		  "model": "deepseek-v4-flash",
		  "thinking": {
		    "type": "enabled"
		  },
		  "reasoning_effort": "high",
		  "max_tokens": 4096,
		  "response_format": {
		    "type": "json_object"
		  },
		  "stop": null,
		  "stream": false,
		  "stream_options": null,
		  "temperature": 0.2,
		  "top_p": 1,
		  "tools": null,
		  "tool_choice": "none",
		  "logprobs": false,
		  "top_logprobs": null
		}
		)"),
	*EscapeJson(SystemPrompt),
	*EscapeJson(Text));
	Request->SetContentAsString(JsonBody);
	
	// 绑定返回回调
	Request->OnProcessRequestComplete().BindRaw(this, &DeepSeekProvider::OnProcessRequestComplete);

	// 真正发送请求
	const bool bStarted = Request->ProcessRequest();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Request started: %s"),
		bStarted ? TEXT("true") : TEXT("false")
	);
	
}

void DeepSeekProvider::SetTranslationPanel(TWeakPtr<STranslationPanel> TranslationPanelWeakPtr)
{
	TranslationPanel = TranslationPanelWeakPtr;
}

void DeepSeekProvider::OnProcessRequestComplete(FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
{
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("DeepSeek request failed"));
			return;
	}

	if (!Res.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("DeepSeek response is invalid"));
		return;
	}
	
	const int32 StatusCode = Res->GetResponseCode();
	const FString ResponseContent =
		Res->GetContentAsString();
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		// choices
		const TArray<TSharedPtr<FJsonValue>>* ChoicesInfos;
		if (JsonObject->TryGetArrayField(TEXT("choices"), ChoicesInfos))
		{
			for (const TSharedPtr<FJsonValue>& Value : *ChoicesInfos)
			{
				const TSharedPtr<FJsonObject> ChoiceObject = Value->AsObject();
				TMap<FJsonObject::FStringType, TSharedPtr<FJsonValue>> Currency = ChoiceObject->Values;
				for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Currency)  
				{  
					TSharedPtr<FJsonObject>* Object;
					FString OutString;
					if (Pair.Value->TryGetObject(Object))
					{
						for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Rabbit : Object->Get()->Values)
						{
							TSharedRef<TJsonReader<>> ContentReader =
							TJsonReaderFactory<>::Create(Rabbit.Value->AsString());
							TSharedPtr<FJsonObject> ContentObject;
							if (FJsonSerializer::Deserialize(ContentReader, ContentObject) &&
								ContentObject.IsValid())
							{
								FString Translation =
									ContentObject->GetStringField(TEXT("translation"));
								
								
								if (Translation.IsEmpty()) Translation = ContentObject->GetStringField(TEXT("best"));;
								 
								UE_LOG(
								   LogTemp,
								   Warning,
								   TEXT("萝卜叫叫叫 %s"), *Translation);
								if (TSharedPtr<STranslationPanel> Panel = TranslationPanel.Pin())
								{
									Panel.Get()->ConstructedSecondMultiLineEditableTextBox->SetText(
										FText::FromString(Translation));
									Panel.Get()->LastResult = Translation;
								}
								
								
								
								TArray<TSharedPtr<FJsonValue>> AlternativesObject = 
									ContentObject->GetArrayField(TEXT("alternatives"));
								for (TSharedPtr<FJsonValue> AlternativeObject:AlternativesObject)
								{
									UE_LOG(
								   LogTemp,
								   Warning,
								   TEXT("Alternatives 叫叫叫 %s"), *AlternativeObject->AsString());
									if (TSharedPtr<STranslationPanel> Panel = TranslationPanel.Pin())
									{
										Panel.Get()->Alternatives.Add(AlternativeObject->AsString());
										TSharedPtr<FString> Alternative = MakeShared<FString>(
											AlternativeObject->AsString());
										Panel.Get()->DisplayedAlternatives.Add(Alternative);
										if (Panel.Get()->ConstructedAlternativesListView.IsValid())
										{
											Panel.Get()->ConstructedAlternativesListView->RebuildList();
										}
										
										if (Panel.Get()->ConstructedSThrobber.IsValid())
										{
											Panel.Get()->ConstructedSThrobber->SetVisibility(EVisibility::Hidden);
										}
									}
								}
							}
							
							UE_LOG(
								   LogTemp,
								   Warning,
								   TEXT("大萝卜叫叫叫 %s"), *Rabbit.Value->AsString());
							
						}
					}
				} 
			}
		}
	}
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Response: %s"),
		*ResponseContent
	);
	if (TSharedPtr<STranslationPanel> Panel = TranslationPanel.Pin())
	{
		if (Panel.Get()->ConstructedSThrobber.IsValid())
		{
			Panel.Get()->ConstructedSThrobber->SetVisibility(EVisibility::Hidden);
		}
	}
}
