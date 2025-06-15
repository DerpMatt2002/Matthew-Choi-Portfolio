import numpy as np
import argparse


def sigmoid(x : np.ndarray):
    """
    Implementation of the sigmoid function.

    Parameters:
        x (np.ndarray): Input np.ndarray.

    Returns:
        An np.ndarray after applying the sigmoid function element-wise to the
        input.
    """
    e = np.exp(x)
    return e / (1 + e)



def dJ(
    theta : np.ndarray, # shape (D,) where D is feature dim
    X : np.ndarray,     # shape (N, D) where N is num of examples
    y : np.ndarray,     # shape (N,)
    i : int
):
    Xi = X[i]
    return (sigmoid(np.dot(np.transpose(theta),Xi)) - y[i])* Xi
    


def train(
    theta : np.ndarray, # shape (D,) where D is feature dim
    X : np.ndarray,     # shape (N, D) where N is num of examples
    y : np.ndarray,     # shape (N,)
    num_epoch : int, 
    learning_rate : float
):
    # TODO: Implement `train` using vectorization

    for epoch in range(num_epoch):
        for i in range(len(X)):
            theta -= learning_rate * dJ(theta,X,y,i)
    return theta




def predict(
    theta : np.ndarray,
    X : np.ndarray
) -> np.ndarray:
    # TODO: Implement `predict` using vectorization

    list = []
    for elem in X:
        #dot product of theta and X
        #print(sigmoid(theta @ elem))
        #print("\n")
        if(sigmoid(theta @ elem) >= 0.5):
            list.append(1)
        else:
            list.append(0)
    return list




def compute_error(
    y_pred : np.ndarray, 
    y : np.ndarray
) -> float:
    # TODO: Implement `compute_error` using vectorization
    pass

    errorcount = 0
    for i in range(len(y_pred)):
        if (y_pred[i] != y[i]):
            errorcount += 1
    return errorcount/len(y_pred)


def create_array(inputdata):
    list = []
    for row in inputdata:
        list.append(row[1:])
    return list

def retrieve_label(inputdata):
    list = []
    for row in inputdata:
        list.append(row[0])
    return list

def write_label(data,outputfile):
    f = open(outputfile,"w")
    for elem in data:
        f.write(str(elem))
        f.write("\n")


if __name__ == '__main__':
    # This takes care of command line argument parsing for you!
    # To access a specific argument, simply access args.<argument name>.
    # For example, to get the learning rate, you can use `args.learning_rate`.
    parser = argparse.ArgumentParser()
    parser.add_argument("train_input", type=str, help='path to formatted training data')
    parser.add_argument("validation_input", type=str, help='path to formatted validation data')
    parser.add_argument("test_input", type=str, help='path to formatted test data')
    parser.add_argument("train_out", type=str, help='file to write train predictions to')
    parser.add_argument("test_out", type=str, help='file to write test predictions to')
    parser.add_argument("metrics_out", type=str, help='file to write metrics to')
    parser.add_argument("num_epoch", type=int, 
                        help='number of epochs of stochastic gradient descent to run')
    parser.add_argument("learning_rate", type=float,
                        help='learning rate for stochastic gradient descent')
    args = parser.parse_args()


    # retrieve the training data, validation data, and test data

    training = np.loadtxt(args.train_input,delimiter='\t')
    testing = np.loadtxt(args.test_input,delimiter='\t')


    trains = create_array(training)
    trainlabel = retrieve_label(training)
    #validlabel,validdata = create_array(valid)
    #testlabel,testdata = create_array(testing)
    tests = create_array(testing)
    testlabel = retrieve_label(testing)

    trainint = [1]*len(trains)
    #validint = [1]*len(validlabel)
    testint = [1]*len(testlabel)



    #add a new column of 1s to all 3
    newtrains = np.insert(trains,0,trainint,axis=1)
    newtests = np.insert(tests,0,testint,axis=1)
    #print(newtrains)
    
    #np.concatenate((validint[:, np.newaxis],validdata),axis=1)
    #np.concatenate((testint[:, np.newaxis],testdata),axis=1)

    #create default theta
    theta = [0]*301

    # run train using train data
    train_t = train(theta,newtrains,trainlabel,args.num_epoch,args.learning_rate)

    train_list = predict(train_t,newtrains)
    #print(train_list)



    trainerror = compute_error(trainlabel,train_list)


    #predict test
    test_list = predict(train_t,newtests)
    #print(test_list)
    #print(testlabel)
    testerror = compute_error(test_list,testlabel)

    write_label(train_list,args.train_out)
    write_label(test_list,args.test_out)



    #modify metrics
    with open(args.metrics_out,'w') as m:
        m.writelines("error(train): {:.6f}\n".format(trainerror))
        m.writelines("error(test): {:.6f}\n".format(testerror))
