import Types

dictionary = []    #the type dictionary

def add(type):
    global dictionary
    dictionary.append(type)

def output(formatter):
    global dictionary
    for type in dictionary: type.output(formatter)
