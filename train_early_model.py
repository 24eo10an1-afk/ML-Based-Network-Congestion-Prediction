import pandas as pd
import joblib
from sklearn.ensemble import RandomForestClassifier

df = pd.read_csv("early_ml_dataset.csv")

features = [
    "TrafficRate",
    "EarlyPacketsSent",
    "EarlyPacketsReceived",
    "EarlyPacketsLost",
    "EarlyDelay",
    "EarlyJitter",
    "EarlyThroughput",
    "EarlyQueue",
    "Utilization"
]

X = df[features]
y = (df["Class"] == "CONGESTION").astype(int)

model = RandomForestClassifier(
    n_estimators=100,
    random_state=42
)

model.fit(X, y)

joblib.dump(model, "early_congestion_model.pkl")

print("Early ML model training completed.")
print("Model saved as: early_congestion_model.pkl")
print("\nClass distribution:")
print(df["Class"].value_counts())
print("\nTraining accuracy:")
print(model.score(X, y))
