import csv
import numpy as np
import argparse

VECTOR_LEN = 300   # Length of glove vector
MAX_WORD_LEN = 64  # Max word length in dict.txt and glove_embeddings.txt

################################################################################
# We have provided you the functions for loading the tsv and txt files. Feel   #
# free to use them! No need to change them at all.                             #
################################################################################


def load_tsv_dataset(file):
    """
    Loads raw data and returns a tuple containing the reviews and their ratings.

    Parameters:
        file (str): File path to the dataset tsv file.

    Returns:
        An np.ndarray of shape N. N is the number of data points in the tsv file.
        Each element dataset[i] is a tuple (label, review), where the label is
        an integer (0 or 1) and the review is a string.
    """
    dataset = np.loadtxt(file, delimiter='\t', comments=None, encoding='utf-8',
                         dtype='l,O')
    return dataset


def load_feature_dictionary(file):
    """
    Creates a map of words to vectors using the file that has the glove
    embeddings.

    Parameters:
        file (str): File path to the glove embedding file.

    Returns:
        A dictionary indexed by words, returning the corresponding glove
        embedding np.ndarray.
    """
    glove_map = dict()
    with open(file, encoding='utf-8') as f:
        read_file = csv.reader(f, delimiter='\t')
        for row in read_file:
            word, embedding = row[0], row[1:]
            glove_map[word] = np.array(embedding, dtype=float)
    return glove_map

def feature(input,dict,output):

    #create a new array
    #print(input)
    with open(output,'w') as m:

        for row in input:
            out = []
            out.append("{:.6f}".format(row[0]))
            out.append('\t')
            #create a zero vector of len 300
            v = [0] * VECTOR_LEN
            #print(v)
            counter = 0


            #take the sentence and read it
            #then check if each word is in the dictionary
            #print(row[1])

            #convert for word in row[1] to list(string.split(" "))

            li = list(row[1].split(" "))
            for word in li:
                #if word is in the dictionary, 
                if word in dict.keys():
                    v += dict.get(word)
                    counter += 1
            #divide v by counter
            v = v/counter

            #print(v)
            
            #round v to be 6 digits
            for elem in v:
                newelem = "{:.6f}".format(elem)
                out.append(newelem)
                out.append('\t')
            #replace last elem \t with \n
            out.pop()
            out.append('\n')

            #print(out)

            #convert array to txt
            txt = ""

            for elem in out:
                txt += elem

            #upload txt to file
            
            m.writelines(txt)
        
        #print(txt)
        
            

        




        





if __name__ == '__main__':
    # This takes care of command line argument parsing for you!
    # To access a specific argument, simply access args.<argument name>.
    # For example, to get the train_input path, you can use `args.train_input`.
    parser = argparse.ArgumentParser()
    parser.add_argument("train_input", type=str, help='path to training input .tsv file')
    parser.add_argument("validation_input", type=str, help='path to validation input .tsv file')
    parser.add_argument("test_input", type=str, help='path to the input .tsv file')
    parser.add_argument("feature_dictionary_in", type=str, 
                        help='path to the GloVe feature dictionary .txt file')
    parser.add_argument("train_out", type=str, 
                        help='path to output .tsv file to which the feature extractions on the training data should be written')
    parser.add_argument("validation_out", type=str, 
                        help='path to output .tsv file to which the feature extractions on the validation data should be written')
    parser.add_argument("test_out", type=str, 
                        help='path to output .tsv file to which the feature extractions on the test data should be written')
    args = parser.parse_args()


    train_in = load_tsv_dataset(args.train_input)
    valid_in = load_tsv_dataset(args.validation_input)
    test_in = load_tsv_dataset(args.test_input)

    glovedict = load_feature_dictionary(args.feature_dictionary_in)

    feature(train_in,glovedict,args.train_out)
    feature(valid_in,glovedict,args.validation_out)
    feature(test_in,glovedict,args.test_out)
