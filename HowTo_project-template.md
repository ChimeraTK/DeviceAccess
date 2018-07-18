This file descibes the intended way to use the template project.

## Concept:
The project template is merged via git. The idea is to have the whole
project-template repository merged into the project which uses it, incl.
all the history. Like this updates and improvements in the template
scripts can easily be propagated to all client projects by simply
merging the head of project template.

## Instructions:

### Preparing your project to use the project template
  -  Add the project template repository as a git remote to your project.
     This only is done once for each local repository.
     ```
     $ git remote add project-template https://github.com/ChimeraTK/project-template.git
     ```
     We intentionally use the read-only https version here. Never write
     to the project template from one of the client projects!

  -  Update the remote on you hard disk so git knows what is in
     project-template
     ```
     $ git remote update
     ```

  -  Merge the project template master into your project
     ```
     $ git merge project-template/master
     ```
     
    Note:you can access the files provided in the cmake directory from your 
    CMakeLists.txt after appending this location to your CMAKE_MODULE_PATH 
    variable

### Getting updates to the project template
  ```
     $ git remote update
     $ git merge project-template/master

  ```
  If you are on a different computer with a different checkout directory which does not have the remote added 
  (check with 'git remote show'), you may have to repeat steps from the 
  section: Preparing your project to use the project     template.
 

### Updating/Improving the project template.

  - Never write to the project template from one of the client projects!  If
    you have improvements or additions to the project template, check out
    the project-template repository separately, make the modifications and
    commit/push them.
  
  - Be careful not to break functionality for other project which are using
    the template!
  
  - After the project template is updated you can merge the changes into
    your project as described in the section above. (Getting updates of project template)
  
  - Note: For testing you might want to add your local repository of
    project-template as a remote to your project, so you don't have to push
    untested changes. Be careful that everything is pushed to github when
    you are done in order not to lose consistency of the repositories.
