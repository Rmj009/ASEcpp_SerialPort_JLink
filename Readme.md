readme!

# 為什麼
```
public static LogSingleton Instance // 很多位置需要用"Instance" 間接來實作? 跟 membership func >> private static LogSingleton mSingleton 有什麼不同?
LogSingleton.Instance.WriteLog(str);
LogSingleton.Instance.WriteLog(errorMsg, LogSingleton.ERROR);
```