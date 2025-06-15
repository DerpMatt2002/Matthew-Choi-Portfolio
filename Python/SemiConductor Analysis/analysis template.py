import pandas as pd
import numpy as np
from sklearn.metrics import accuracy_score
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from xgboost import XGBClassifier
from lightgbm import LGBMClassifier
from sklearn.impute import SimpleImputer
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import OneHotEncoder
from sklearn.preprocessing import LabelEncoder

# Function to get the model
def get_model(model_algo):
    """
    Returns a model based on the specified algorithm.
    """
    if model_algo == "logistic":
        model = LogisticRegression()
    elif model_algo == "xgboost":
        model = XGBClassifier()
    elif model_algo == "lightgbm":
        model = LGBMClassifier(n_jobs=1)
    elif model_algo == "forest":
        model = RandomForestClassifier()
    else:
        raise ValueError("Invalid model type. Choose from 'lightgbm', 'xgboost', 'forest', 'logistic'")
    return model

# Function for feature engineering
def feature_eng(df):
    """
    Performs basic feature engineering: imputes missing values for numeric columns only.
    """
    # Select only numeric columns
    numeric_cols = df.select_dtypes(include=['number']).columns.tolist()
    
    # Handle empty numeric columns case
    if not numeric_cols:
        return pd.DataFrame()
    
    # Impute missing values
    imputer = SimpleImputer(strategy='mean')
    numeric_data = imputer.fit_transform(df[numeric_cols])
    
    # Create DataFrame with correct column count
    df_processed = pd.DataFrame(
        numeric_data, 
        columns=numeric_cols[:numeric_data.shape[1]]
    )
    
    # Convert all columns to float explicitly
    df_processed = df_processed.astype(float)
    
    return df_processed



# Function for feature importance
def feature_imp(model_algo, model, input_cols):
    """
    Returns a dataframe with input features and their importance.
    """
    if model_algo in ["xgboost", "lightgbm", "forest"]:
        importance = model.feature_importances_
    elif model_algo == "logistic":
        importance = np.abs(model.coef_[0])
    else:
        raise ValueError("Model algorithm not supported for feature importance extraction.")
    
    importance_df = pd.DataFrame({'Feature': input_cols, 'Importance': importance})
    importance_df = importance_df.sort_values(by='Importance', ascending=False)
    
    return importance_df

# Function to compute correlation with response variable
def correlate(df, response):
    """
    Returns a dataframe of correlations between input features and the response variable.
    
    """
    if not isinstance(response, pd.Series):
        response = pd.Series(response)
    corr = df.corrwith(response)
    corr_df = pd.DataFrame({'Feature': df.columns, 'Correlation': corr})
    corr_df = corr_df.sort_values(by='Correlation', ascending=False)
    
    return corr_df

# Function to fit the ML model and get predictions
def ml_fit(df, response, model_algo):
    """
    Fits a machine learning model, evaluates performance, and generates outputs.
    """
    # Drop the target column from the feature set

    df_features = df.drop(columns=[response])
    
    # Feature engineering
    df_processed = feature_eng(df_features).astype(float)
    
       
    # Extract the target column as y
    y = df[response].astype(float)

    # Encode labels to 0-based integers
    label_encoder = LabelEncoder()
    y_encoded = label_encoder.fit_transform(y)  # Converts [1.0, 2.0] → [0, 1]
    
    # Split the data
    X_train, X_test, y_train, y_test = train_test_split(
        df_processed, y_encoded, test_size=0.2, random_state=42
    )
    
    # Get the model
    model = get_model(model_algo)
    
    # Fit the model
    model.fit(X_train, y_train)
    
    # Get predictions
    y_pred = model.predict(X_test)
    
    # Calculate accuracy score
    score = accuracy_score(y_test, y_pred)
    
    # Get feature importance
    importance_df = feature_imp(model_algo, model, X_train.columns)
    
    # Get correlation DataFrame
    corr_df = correlate(df_processed, df[response].astype(float))
    
    # Return results
    return score, y_pred, importance_df, corr_df


score, result_df, importance_df, corr_df = ml_fit(dfData, response, model_algo)
