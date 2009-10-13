// -*- C++ -*-

#ifndef glclass_h__
#define glclass_h__

#include <typeinfo>
#include <stdio.h>
#include "gliovecs.h"
#include "gldataset.h"

namespace {
    // compute a classmap that maps a set of possibly sparse classes onto a dense
    // list of new classes and vice versa

    void classmap(intarray &class_to_index,intarray &index_to_class,intarray &classes) {
        int nclasses = max(classes)+1;
        intarray hist(nclasses);
        hist.fill(0);
        for(int i=0;i<classes.length();i++) {
            if(classes(i)==-1) continue;
            hist(classes(i))++;
        }
        int count = 0;
        for(int i=0;i<hist.length();i++)
            if(hist(i)>0) count++;
        class_to_index.resize(nclasses);
        class_to_index.fill(-1);
        index_to_class.resize(count);
        index_to_class.fill(-1);
        int index=0;
        for(int i=0;i<hist.length();i++) {
            if(hist(i)>0) {
                class_to_index(i) = index;
                index_to_class(index) = i;
                index++;
            }
        }
        CHECK(class_to_index.length()==nclasses);
        CHECK(index_to_class.length()==max(class_to_index)+1);
        CHECK(index_to_class.length()<=class_to_index.length());
    }

    // unpack posteriors or discriminant values using a translation map
    // this is to go from the output of a classifier with a limited set
    // of classes to a full output vector

    void ctranslate_vec(floatarray &result,floatarray &input,intarray &translation) {
        result.resize(max(translation)+1);
        result.fill(0);
        for(int i=0;i<input.length();i++)
            result(translation(i)) = input(i);
    }

    void ctranslate_vec(floatarray &v,intarray &translation) {
        floatarray temp;
        ctranslate_vec(temp,v,translation);
        v.move(temp);
    }

    // translate classes using a translation map

    void ctranslate(intarray &values,intarray &translation) {
        for(int i=0;i<values.length();i++)
            values(i) = translation(values(i));
    }

    void ctranslate(intarray &result,intarray &values,intarray &translation) {
        result.resize(values.length());
        for(int i=0;i<values.length();i++) {
            int v = values(i);
            if(v<0) result(i) = v;
            else result(i) = translation(v);
        }
    }

    // count the number of distinct classes in a list of classes (also
    // works for a translation map

    int classcount(intarray &classes) {
        int nclasses = max(classes)+1;
        intarray hist(nclasses);
        hist.fill(0);
        for(int i=0;i<classes.length();i++) {
            if(classes(i)==-1) continue;
            hist(classes(i))++;
        }
        int count = 0;
        for(int i=0;i<hist.length();i++)
            if(hist(i)>0) count++;
        return count;
    }

}

namespace glinerec {

    struct IModel : IComponent {
        IModel() {
            pdef("cds","rowdataset8","default dataset buffer class");
        }

        virtual const char *name() { return "IModel"; }
        virtual const char *interface() { return "IModel"; }

        // inquiry functions
        virtual int nfeatures() {throw Unimplemented();}
        virtual int nclasses() {throw Unimplemented();}
        virtual float complexity() {return 1.0;}

        // submodels
        virtual int nmodels() { return 0; }
        virtual void setModel(IModel *,int i) { throw "no submodels"; }
        virtual IComponent &getModel(int i) { throw "no submodels"; }

        // update this model in place
        virtual void copy(IModel &) { throw Unimplemented(); }

        float outputs(OutputVector &result,InputVector &v) {
            return outputs_impl(result,v);
        }
        float outputs(floatarray &result,floatarray &v) {
            OutputVector result_(result);
            InputVector v_(v);
            return outputs_impl(result_,v_);
        }

        virtual float outputs_impl(OutputVector &result,InputVector &v) {
            floatarray result_;
            floatarray v_;
            v.ravel(v_);
            float value = outputs_impl(result_,v_);
            result.copy(result_);
            return value;
        }
        virtual float outputs_impl(floatarray &result,floatarray &v) {
            throw Unimplemented();
        }
    public:
#if 0
        float outputs(OutputVector &result,floatarray &v) {
            InputVector v_;
            return outputs(result,v_);
        }
        float outputs(floatarray &result,InputVector &v) {
            OutputVector result_(result);
            return outputs(result_,v);
        }
        float outputs(floatarray &result,floatarray &v) {
            OutputVector result_(result);
            InputVector v_(v);
            return outputs(result_,v_);
        }
#endif

        virtual float cost(floatarray &v) {
            OutputVector temp;
            InputVector v_(v);
            return outputs(temp,v_);
        }

        // convenience function
        virtual int classify(floatarray &v) {
            OutputVector p;
            InputVector v_(v);
            outputs(p,v_);
            return p.argmax();
        }

        // estimate the cross validated error from the training data seen
        virtual float crossValidatedError() { throw Unimplemented(); }

        // batch training with dataset interface
        virtual void train(IDataset &dataset) = 0;

        // incremental training & default implementation in terms of batch
        autodel<IExtDataset> ds;
        virtual void add(floatarray &v,int c) {
            if(!ds) {
                debugf("info","allocating %s buffer for classifier\n",pget("cds"));
                make_component(pget("cds"),ds);
            }
            ds->add(v,c);
        }
        virtual void updateModel() {
            if(!ds) return;
            debugf("info","updateModel %d samples, %d features, %d classes\n",
                   ds->nsamples(),ds->nfeatures(),ds->nclasses());
            debugf("info","updateModel memory status %d Mbytes, %d Mvalues\n",
                   int((long long)sbrk(0)/1000000),
                   (ds->nsamples() * ds->nfeatures())/1000000);
            train(*ds);
            ds = 0;
        }
    };

    ////////////////////////////////////////////////////////////////
    // MappedClassifier remaps classes prior to classification.
    ////////////////////////////////////////////////////////////////

    struct MappedClassifier : IModel {
        autodel<IModel> cf;
        intarray c2i,i2c;
        MappedClassifier() {
        }
        MappedClassifier(IModel *cf):cf(cf) {
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"MappedClassifier\n");
            pprint(stream,depth);
            iprintf(stream,depth,"mapping from %d to %d dimensions\n",
                   c2i.dim(0),i2c.dim(0));
            if(!!cf) cf->info(depth+1);
        }
        int nmodels() {
            return 1;
        }
        void setModel(IModel *cf,int which) {
            this->cf = cf;
        }
        IModel &getModel(int i) {
            return *cf;
        }
        int nfeatures() {
            return cf->nfeatures();
        }
        int nclasses() {
            return c2i.length();
        }
        const char *name() {
            return "mapped";
        }
        void save(FILE *stream) {
            magic_write(stream,"mapped");
            psave(stream);
            narray_write(stream,c2i);
            narray_write(stream,i2c);
            // cf->save(stream);
            save_component(stream,cf.ptr());
        }

        void load(FILE *stream) {
            magic_read(stream,"mapped");
            pload(stream);
            narray_read(stream,c2i);
            narray_read(stream,i2c);
            // cf->load(stream);
            cf = dynamic_cast<IModel*>(load_component(stream));
            CHECK_ARG(!!cf);
        }

        void pset(const char *name,const char *value) {
            if(pexists(name)) pset(name,value);
            if(cf->pexists(name)) cf->pset(name,value);
        }
        void pset(const char *name,double value) {
            if(pexists(name)) pset(name,value);
            if(cf->pexists(name)) cf->pset(name,value);
        }
        int classify(floatarray &x) {
            return i2c(cf->classify(x));
        }
        float outputs_impl(OutputVector &z,InputVector &x) {
            float result = cf->outputs(z,x);
            ctranslate_vec(z,i2c);
            return result;
        }

        static void hist(intarray &h,intarray &a) {
            h.resize(max(a)+1);
            h = 0;
            for(int i=0;i<a.length();i++)
                h(a(i))++;
        }

        // Note: this is different from the toplevel MappedDataset

        struct MappedDataset : IDataset {
            IDataset &ds;
            intarray &c2i;
            int nc;
            const char *name() { return "mappeddataset"; }
            MappedDataset(IDataset &ds,intarray &c2i) : ds(ds),c2i(c2i) {
                nc = max(c2i)+1;
            }
            int nclasses() { return nc; }
            int nfeatures() { return ds.nfeatures(); }
            int nsamples() { return ds.nsamples(); }
            void input(floatarray &v,int i) { ds.input(v,i); }
            int cls(int i) { return c2i(ds.cls(i)); }
            int id(int i) { return ds.id(i); }
        };

        void train(IDataset &ds) {
            CHECK(ds.nsamples()>0);
            CHECK(ds.nfeatures()>0);
            if(c2i.length()<1) {
                intarray raw_classes;
                for(int i=0;i<ds.nsamples();i++)
                    raw_classes.push(ds.cls(i));
                classmap(c2i,i2c,raw_classes);
                intarray classes;
                ctranslate(classes,raw_classes,c2i);
                debugf("info","[mapped %d to %d classes]\n",c2i.length(),i2c.length());
            }
            MappedDataset mds(ds,c2i);
            cf->train(mds);
        }
    };


    void confusion_matrix(intarray &confusion,IModel &classifier,floatarray &data,intarray &classes);
    void confusion_matrix(intarray &confusion,IModel &classifier,IDataset &ds);
    void confusion_print(intarray &confusion);
    int compute_confusions(intarray &list,intarray &confusion);
    void print_confusion(IModel &classifier,floatarray &vs,intarray &cs);
    float confusion_error(intarray &confusion);

    void least_square(floatarray &xf,floatarray &Af,floatarray &bf);

    inline IModel *make_model(const char *name) {
        IModel *result = dynamic_cast<IModel*>(component_construct(name));
        CHECK(result!=0);
        return result;
    }

    extern IRecognizeLine *current_recognizer_;
}

#endif
